
#include "config.h"

#include "helpers.h"

#include <algorithm>
#include <cerrno>
#include <cstdarg>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <limits>
#include <string>
#include <tuple>

#include "almalloc.h"
#include "alfstream.h"
#include "alnumeric.h"
#include "aloptional.h"
#include "alspan.h"
#include "alstring.h"
#include "logging.h"
#include "strutils.h"
#include "vector.h"


/* Mixing thread piority level */
int RTPrioLevel{1};

/* Allow reducing the process's RTTime limit for RTKit. */
bool AllowRTTimeLimit{true};


#ifdef _WIN32

#include <shlobj.h>

const PathNamePair &GetProcBinary()
{
    static al::optional<PathNamePair> procbin;
    if(procbin) return *procbin;

    auto fullpath = al::vector<WCHAR>(256);
    DWORD len{GetModuleFileNameW(nullptr, fullpath.data(), static_cast<DWORD>(fullpath.size()))};
    while(len == fullpath.size())
    {
        fullpath.resize(fullpath.size() << 1);
        len = GetModuleFileNameW(nullptr, fullpath.data(), static_cast<DWORD>(fullpath.size()));
    }
    if(len == 0)
    {
        ERR("Failed to get process name: error %lu\n", GetLastError());
        procbin = al::make_optional<PathNamePair>();
        return *procbin;
    }

    fullpath.resize(len);
    if(fullpath.back() != 0)
        fullpath.push_back(0);

    auto sep = std::find(fullpath.rbegin()+1, fullpath.rend(), '\\');
    sep = std::find(fullpath.rbegin()+1, sep, '/');
    if(sep != fullpath.rend())
    {
        *sep = 0;
        procbin = al::make_optional<PathNamePair>(wstr_to_utf8(fullpath.data()),
            wstr_to_utf8(&*sep + 1));
    }
    else
        procbin = al::make_optional<PathNamePair>(std::string{}, wstr_to_utf8(fullpath.data()));

    TRACE("Got binary: %s, %s\n", procbin->path.c_str(), procbin->fname.c_str());
    return *procbin;
}

namespace {

void DirectorySearch(const char *path, const char *ext, al::vector<std::string> *const results)
{
    std::string pathstr{path};
    pathstr += "\\*";
    pathstr += ext;
    TRACE("Searching %s\n", pathstr.c_str());

    std::wstring wpath{utf8_to_wstr(pathstr.c_str())};
    WIN32_FIND_DATAW fdata;
    HANDLE hdl{FindFirstFileW(wpath.c_str(), &fdata)};
    if(hdl == INVALID_HANDLE_VALUE) return;

    const auto base = results->size();

    do {
        results->emplace_back();
        std::string &str = results->back();
        str = path;
        str += '\\';
        str += wstr_to_utf8(fdata.cFileName);
    } while(FindNextFileW(hdl, &fdata));
    FindClose(hdl);

    const al::span<std::string> newlist{results->data()+base, results->size()-base};
    std::sort(newlist.begin(), newlist.end());
    for(const auto &name : newlist)
        TRACE(" got %s\n", name.c_str());
}

} // namespace

al::vector<std::string> SearchDataFiles(const char *ext, const char *subdir)
{
    auto is_slash = [](int c) noexcept -> int { return (c == '\\' || c == '/'); };

    static std::mutex search_lock;
    std::lock_guard<std::mutex> _{search_lock};

    /* If the path is absolute, use it directly. */
    al::vector<std::string> results;
    if(isalpha(subdir[0]) && subdir[1] == ':' && is_slash(subdir[2]))
    {
        std::string path{subdir};
        std::replace(path.begin(), path.end(), '/', '\\');
        DirectorySearch(path.c_str(), ext, &results);
        return results;
    }
    if(subdir[0] == '\\' && subdir[1] == '\\' && subdir[2] == '?' && subdir[3] == '\\')
    {
        DirectorySearch(subdir, ext, &results);
        return results;
    }

    std::string path;

    /* Search the app-local directory. */
    if(auto localpath = al::getenv(L"ALSOFT_LOCAL_PATH"))
    {
        path = wstr_to_utf8(localpath->c_str());
        if(is_slash(path.back()))
            path.pop_back();
    }
    else if(WCHAR *cwdbuf{_wgetcwd(nullptr, 0)})
    {
        path = wstr_to_utf8(cwdbuf);
        if(is_slash(path.back()))
            path.pop_back();
        free(cwdbuf);
    }
    else
        path = ".";
    std::replace(path.begin(), path.end(), '/', '\\');
    DirectorySearch(path.c_str(), ext, &results);

    /* Search the local and global data dirs. */
    static const int ids[2]{ CSIDL_APPDATA, CSIDL_COMMON_APPDATA };
    for(int id : ids)
    {
        WCHAR buffer[MAX_PATH];
        if(SHGetSpecialFolderPathW(nullptr, buffer, id, FALSE) == FALSE)
            continue;

        path = wstr_to_utf8(buffer);
        if(!is_slash(path.back()))
            path += '\\';
        path += subdir;
        std::replace(path.begin(), path.end(), '/', '\\');

        DirectorySearch(path.c_str(), ext, &results);
    }

    return results;
}

void SetRTPriority(void)
{
    if(RTPrioLevel > 0)
    {
        if(!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL))
            ERR("Failed to set priority level for thread\n");
    }
}

#else

#include <sys/types.h>
#include <unistd.h>
#include <dirent.h>
#ifdef __FreeBSD__
#include <sys/sysctl.h>
#endif
#ifdef __HAIKU__
#include <FindDirectory.h>
#endif
#ifdef HAVE_PROC_PIDPATH
#include <libproc.h>
#endif
#if defined(HAVE_PTHREAD_SETSCHEDPARAM) && !defined(__OpenBSD__)
#include <pthread.h>
#include <sched.h>
#endif
#ifdef HAVE_RTKIT
#include <sys/time.h>
#include <sys/resource.h>

#include "dbus_wrap.h"
#include "rtkit.h"
#ifndef RLIMIT_RTTIME
#define RLIMIT_RTTIME 15
#endif
#endif

const PathNamePair &GetProcBinary()
{
    static al::optional<PathNamePair> procbin;
    if(procbin) return *procbin;

    al::vector<char> pathname;
#ifdef __FreeBSD__
    size_t pathlen;
    int mib[4] = { CTL_KERN, KERN_PROC, KERN_PROC_PATHNAME, -1 };
    if(sysctl(mib, 4, nullptr, &pathlen, nullptr, 0) == -1)
        WARN("Failed to sysctl kern.proc.pathname: %s\n", strerror(errno));
    else
    {
        pathname.resize(pathlen + 1);
        sysctl(mib, 4, pathname.data(), &pathlen, nullptr, 0);
        pathname.resize(pathlen);
    }
#endif
#ifdef HAVE_PROC_PIDPATH
    if(pathname.empty())
    {
        char procpath[PROC_PIDPATHINFO_MAXSIZE]{};
        const pid_t pid{getpid()};
        if(proc_pidpath(pid, procpath, sizeof(procpath)) < 1)
            ERR("proc_pidpath(%d, ...) failed: %s\n", pid, strerror(errno));
        else
            pathname.insert(pathname.end(), procpath, procpath+strlen(procpath));
    }
#endif
#ifdef __HAIKU__
    if(pathname.empty())
    {
        char procpath[PATH_MAX];
        if(find_path(B_APP_IMAGE_SYMBOL, B_FIND_PATH_IMAGE_PATH, NULL, procpath, sizeof(procpath)) == B_OK)
            pathname.insert(pathname.end(), procpath, procpath+strlen(procpath));
    }
#endif
#ifndef __SWITCH__
    if(pathname.empty())
    {
        static const char SelfLinkNames[][32]{
            "/proc/self/exe",
            "/proc/self/file",
            "/proc/curproc/exe",
            "/proc/curproc/file"
        };

        pathname.resize(256);

        const char *selfname{};
        ssize_t len{};
        for(const char *name : SelfLinkNames)
        {
            selfname = name;
            len = readlink(selfname, pathname.data(), pathname.size());
            if(len >= 0 || errno != ENOENT) break;
        }

        while(len > 0 && static_cast<size_t>(len) == pathname.size())
        {
            pathname.resize(pathname.size() << 1);
            len = readlink(selfname, pathname.data(), pathname.size());
        }
        if(len <= 0)
        {
            WARN("Failed to readlink %s: %s\n", selfname, strerror(errno));
            len = 0;
        }

        pathname.resize(static_cast<size_t>(len));
    }
#endif
    while(!pathname.empty() && pathname.back() == 0)
        pathname.pop_back();

    auto sep = std::find(pathname.crbegin(), pathname.crend(), '/');
    if(sep != pathname.crend())
        procbin = al::make_optional<PathNamePair>(std::string(pathname.cbegin(), sep.base()-1),
            std::string(sep.base(), pathname.cend()));
    else
        procbin = al::make_optional<PathNamePair>(std::string{},
            std::string(pathname.cbegin(), pathname.cend()));

    TRACE("Got binary: \"%s\", \"%s\"\n", procbin->path.c_str(), procbin->fname.c_str());
    return *procbin;
}

namespace {

void DirectorySearch(const char *path, const char *ext, al::vector<std::string> *const results)
{
    TRACE("Searching %s for *%s\n", path, ext);
    DIR *dir{opendir(path)};
    if(!dir) return;

    const auto base = results->size();
    const size_t extlen{strlen(ext)};

    while(struct dirent *dirent{readdir(dir)})
    {
        if(strcmp(dirent->d_name, ".") == 0 || strcmp(dirent->d_name, "..") == 0)
            continue;

        const size_t len{strlen(dirent->d_name)};
        if(len <= extlen) continue;
        if(al::strcasecmp(dirent->d_name+len-extlen, ext) != 0)
            continue;

        results->emplace_back();
        std::string &str = results->back();
        str = path;
        if(str.back() != '/')
            str.push_back('/');
        str += dirent->d_name;
    }
    closedir(dir);

    const al::span<std::string> newlist{results->data()+base, results->size()-base};
    std::sort(newlist.begin(), newlist.end());
    for(const auto &name : newlist)
        TRACE(" got %s\n", name.c_str());
}

} // namespace

al::vector<std::string> SearchDataFiles(const char *ext, const char *subdir)
{
    static std::mutex search_lock;
    std::lock_guard<std::mutex> _{search_lock};

    al::vector<std::string> results;
    if(subdir[0] == '/')
    {
        DirectorySearch(subdir, ext, &results);
        return results;
    }

    /* Search the app-local directory. */
    if(auto localpath = al::getenv("ALSOFT_LOCAL_PATH"))
        DirectorySearch(localpath->c_str(), ext, &results);
    else
    {
        al::vector<char> cwdbuf(256);
        while(!getcwd(cwdbuf.data(), cwdbuf.size()))
        {
            if(errno != ERANGE)
            {
                cwdbuf.clear();
                break;
            }
            cwdbuf.resize(cwdbuf.size() << 1);
        }
        if(cwdbuf.empty())
            DirectorySearch(".", ext, &results);
        else
        {
            DirectorySearch(cwdbuf.data(), ext, &results);
            cwdbuf.clear();
        }
    }

    // Search local data dir
    if(auto datapath = al::getenv("XDG_DATA_HOME"))
    {
        std::string &path = *datapath;
        if(path.back() != '/')
            path += '/';
        path += subdir;
        DirectorySearch(path.c_str(), ext, &results);
    }
    else if(auto homepath = al::getenv("HOME"))
    {
        std::string &path = *homepath;
        if(path.back() == '/')
            path.pop_back();
        path += "/.local/share/";
        path += subdir;
        DirectorySearch(path.c_str(), ext, &results);
    }

    // Search global data dirs
    std::string datadirs{al::getenv("XDG_DATA_DIRS").value_or("/usr/local/share/:/usr/share/")};

    size_t curpos{0u};
    while(curpos < datadirs.size())
    {
        size_t nextpos{datadirs.find(':', curpos)};

        std::string path{(nextpos != std::string::npos) ?
            datadirs.substr(curpos, nextpos++ - curpos) : datadirs.substr(curpos)};
        curpos = nextpos;

        if(path.empty()) continue;
        if(path.back() != '/')
            path += '/';
        path += subdir;

        DirectorySearch(path.c_str(), ext, &results);
    }

    return results;
}

namespace {

bool SetRTPriorityPthread(int prio)
{
    int err{ENOTSUP};
#if defined(HAVE_PTHREAD_SETSCHEDPARAM) && !defined(__OpenBSD__)
    /* Get the min and max priority for SCHED_RR. Limit the max priority to
     * half, for now, to ensure the thread can't take the highest priority and
     * go rogue.
     */
    int rtmin{sched_get_priority_min(SCHED_RR)};
    int rtmax{sched_get_priority_max(SCHED_RR)};
    rtmax = (rtmax-rtmin)/2 + rtmin;

    struct sched_param param{};
    param.sched_priority = clampi(prio, rtmin, rtmax);
#ifdef SCHED_RESET_ON_FORK
    err = pthread_setschedparam(pthread_self(), SCHED_RR|SCHED_RESET_ON_FORK, &param);
    if(err == EINVAL)
#endif
        err = pthread_setschedparam(pthread_self(), SCHED_RR, &param);
    if(err == 0) return true;

#else

    std::ignore = prio;
#endif
    WARN("pthread_setschedparam failed: %s (%d)\n", std::strerror(err), err);
    return false;
}

bool SetRTPriorityRTKit(int prio)
{
#ifdef HAVE_RTKIT
    if(!HasDBus())
    {
        WARN("D-Bus not available\n");
        return false;
    }
    dbus::Error error;
    dbus::ConnectionPtr conn{dbus_bus_get(DBUS_BUS_SYSTEM, &error.get())};
    if(!conn)
    {
        WARN("D-Bus connection failed with %s: %s\n", error->name, error->message);
        return false;
    }

    /* Don't stupidly exit if the connection dies while doing this. */
    dbus_connection_set_exit_on_disconnect(conn.get(), false);

    auto limit_rttime = [](DBusConnection *c) -> int
    {
        using ulonglong = unsigned long long;
        long long maxrttime{rtkit_get_rttime_usec_max(c)};
        if(maxrttime <= 0) return static_cast<int>(std::abs(maxrttime));
        const ulonglong umaxtime{static_cast<ulonglong>(maxrttime)};

        struct rlimit rlim{};
        if(getrlimit(RLIMIT_RTTIME, &rlim) != 0)
            return errno;

        TRACE("RTTime max: %llu (hard: %llu, soft: %llu)\n", umaxtime, ulonglong{rlim.rlim_max},
            ulonglong{rlim.rlim_cur});
        if(rlim.rlim_max > umaxtime)
        {
            rlim.rlim_max = static_cast<rlim_t>(umaxtime);
            rlim.rlim_cur = std::min(rlim.rlim_cur, rlim.rlim_max);
            if(setrlimit(RLIMIT_RTTIME, &rlim) != 0)
                return errno;
        }
        return 0;
    };

    int nicemin{};
    int err{rtkit_get_min_nice_level(conn.get(), &nicemin)};
    if(err == -ENOENT)
    {
        err = std::abs(err);
        ERR("Could not query RTKit: %s (%d)\n", std::strerror(err), err);
        return false;
    }
    int rtmax{rtkit_get_max_realtime_priority(conn.get())};
    TRACE("Maximum real-time priority: %d, minimum niceness: %d\n", rtmax, nicemin);

    if(rtmax > 0)
    {
        if(AllowRTTimeLimit)
        {
            err = limit_rttime(conn.get());
            if(err != 0)
                WARN("Failed to set RLIMIT_RTTIME for RTKit: %s (%d)\n",
                    std::strerror(err), err);
        }

        /* Limit the maximum real-time priority to half. */
        rtmax = (rtmax+1)/2;
        prio = clampi(prio, 1, rtmax);

        TRACE("Making real-time with priority %d (max: %d)\n", prio, rtmax);
        err = rtkit_make_realtime(conn.get(), 0, prio);
        if(err == 0) return true;

        err = std::abs(err);
        WARN("Failed to set real-time priority: %s (%d)\n", std::strerror(err), err);
    }
    /* Don't try to set the niceness for non-Linux systems. Standard POSIX has
     * niceness as a per-process attribute, while the intent here is for the
     * audio processing thread only to get a priority boost. Currently only
     * Linux is known to have per-thread niceness.
     */
#ifdef __linux__
    if(nicemin < 0)
    {
        TRACE("Making high priority with niceness %d\n", nicemin);
        err = rtkit_make_high_priority(conn.get(), 0, nicemin);
        if(err == 0) return true;

        err = std::abs(err);
        WARN("Failed to set high priority: %s (%d)\n", std::strerror(err), err);
    }
#endif /* __linux__ */

#else

    std::ignore = prio;
    WARN("D-Bus not supported\n");
#endif
    return false;
}

} // namespace

void SetRTPriority()
{
    if(RTPrioLevel <= 0)
        return;

    if(SetRTPriorityPthread(RTPrioLevel))
        return;
    if(SetRTPriorityRTKit(RTPrioLevel))
        return;
}

#endif
