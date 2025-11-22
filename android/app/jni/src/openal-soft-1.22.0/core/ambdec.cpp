
#include "config.h"

#include "ambdec.h"

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <iterator>
#include <sstream>
#include <string>

#include "alfstream.h"
#include "core/logging.h"


namespace {

template<typename T, std::size_t N>
constexpr inline std::size_t size(const T(&)[N]) noexcept
{ return N; }

int readline(std::istream &f, std::string &output)
{
    while(f.good() && f.peek() == '\n')
        f.ignore();

    return std::getline(f, output) && !output.empty();
}

bool read_clipped_line(std::istream &f, std::string &buffer)
{
    while(readline(f, buffer))
    {
        std::size_t pos{0};
        while(pos < buffer.length() && std::isspace(buffer[pos]))
            pos++;
        buffer.erase(0, pos);

        std::size_t cmtpos{buffer.find_first_of('#')};
        if(cmtpos < buffer.length())
            buffer.resize(cmtpos);
        while(!buffer.empty() && std::isspace(buffer.back()))
            buffer.pop_back();

        if(!buffer.empty())
            return true;
    }
    return false;
}


std::string read_word(std::istream &f)
{
    std::string ret;
    f >> ret;
    return ret;
}

bool is_at_end(const std::string &buffer, std::size_t endpos)
{
    while(endpos < buffer.length() && std::isspace(buffer[endpos]))
        ++endpos;
    return !(endpos < buffer.length());
}


al::optional<std::string> load_ambdec_speakers(AmbDecConf::SpeakerConf *spkrs,
    const std::size_t num_speakers, std::istream &f, std::string &buffer)
{
    size_t cur_speaker{0};
    while(cur_speaker < num_speakers)
    {
        std::istringstream istr{buffer};

        std::string cmd{read_word(istr)};
        if(cmd.empty())
        {
            if(!read_clipped_line(f, buffer))
                return al::make_optional<std::string>("Unexpected end of file");
            continue;
        }

        if(cmd == "add_spkr")
        {
            AmbDecConf::SpeakerConf &spkr = spkrs[cur_speaker++];
            const size_t spkr_num{cur_speaker};

            istr >> spkr.Name;
            if(istr.fail()) WARN("Name not specified for speaker %zu\n", spkr_num);
            istr >> spkr.Distance;
            if(istr.fail()) WARN("Distance not specified for speaker %zu\n", spkr_num);
            istr >> spkr.Azimuth;
            if(istr.fail()) WARN("Azimuth not specified for speaker %zu\n", spkr_num);
            istr >> spkr.Elevation;
            if(istr.fail()) WARN("Elevation not specified for speaker %zu\n", spkr_num);
            istr >> spkr.Connection;
            if(istr.fail()) TRACE("Connection not specified for speaker %zu\n", spkr_num);
        }
        else
            return al::make_optional("Unexpected speakers command: "+cmd);

        istr.clear();
        const auto endpos = static_cast<std::size_t>(istr.tellg());
        if(!is_at_end(buffer, endpos))
            return al::make_optional("Extra junk on line: " + buffer.substr(endpos));
        buffer.clear();
    }

    return al::nullopt;
}

al::optional<std::string> load_ambdec_matrix(float (&gains)[MaxAmbiOrder+1],
    AmbDecConf::CoeffArray *matrix, const std::size_t maxrow, std::istream &f, std::string &buffer)
{
    bool gotgains{false};
    std::size_t cur{0u};
    while(cur < maxrow)
    {
        std::istringstream istr{buffer};

        std::string cmd{read_word(istr)};
        if(cmd.empty())
        {
            if(!read_clipped_line(f, buffer))
                return al::make_optional<std::string>("Unexpected end of file");
            continue;
        }

        if(cmd == "order_gain")
        {
            std::size_t curgain{0u};
            float value;
            while(istr.good())
            {
                istr >> value;
                if(istr.fail()) break;
                if(!istr.eof() && !std::isspace(istr.peek()))
                    return al::make_optional("Extra junk on gain "+std::to_string(curgain+1)+": "+
                        buffer.substr(static_cast<std::size_t>(istr.tellg())));
                if(curgain < size(gains))
                    gains[curgain++] = value;
            }
            std::fill(std::begin(gains)+curgain, std::end(gains), 0.0f);
            gotgains = true;
        }
        else if(cmd == "add_row")
        {
            AmbDecConf::CoeffArray &mtxrow = matrix[cur++];
            std::size_t curidx{0u};
            float value{};
            while(istr.good())
            {
                istr >> value;
                if(istr.fail()) break;
                if(!istr.eof() && !std::isspace(istr.peek()))
                    return al::make_optional("Extra junk on matrix element "+
                        std::to_string(curidx)+"x"+std::to_string(cur-1)+": "+
                        buffer.substr(static_cast<std::size_t>(istr.tellg())));
                if(curidx < mtxrow.size())
                    mtxrow[curidx++] = value;
            }
            std::fill(mtxrow.begin()+curidx, mtxrow.end(), 0.0f);
        }
        else
            return al::make_optional("Unexpected matrix command: "+cmd);

        istr.clear();
        const auto endpos = static_cast<std::size_t>(istr.tellg());
        if(!is_at_end(buffer, endpos))
            return al::make_optional("Extra junk on line: " + buffer.substr(endpos));
        buffer.clear();
    }

    if(!gotgains)
        return al::make_optional<std::string>("Matrix order_gain not specified");
    return al::nullopt;
}

} // namespace

AmbDecConf::~AmbDecConf() = default;


al::optional<std::string> AmbDecConf::load(const char *fname) noexcept
{
    al::ifstream f{fname};
    if(!f.is_open())
        return al::make_optional<std::string>("Failed to open file");

    bool speakers_loaded{false};
    bool matrix_loaded{false};
    bool lfmatrix_loaded{false};
    std::string buffer;
    while(read_clipped_line(f, buffer))
    {
        std::istringstream istr{buffer};

        std::string command{read_word(istr)};
        if(command.empty())
            return al::make_optional("Malformed line: "+buffer);

        if(command == "/description")
            readline(istr, Description);
        else if(command == "/version")
        {
            istr >> Version;
            if(!istr.eof() && !std::isspace(istr.peek()))
                return al::make_optional("Extra junk after version: " +
                    buffer.substr(static_cast<std::size_t>(istr.tellg())));
            if(Version != 3)
                return al::make_optional("Unsupported version: "+std::to_string(Version));
        }
        else if(command == "/dec/chan_mask")
        {
            if(ChanMask)
                return al::make_optional<std::string>("Duplicate chan_mask definition");

            istr >> std::hex >> ChanMask >> std::dec;
            if(!istr.eof() && !std::isspace(istr.peek()))
                return al::make_optional("Extra junk after mask: " +
                    buffer.substr(static_cast<std::size_t>(istr.tellg())));

            if(!ChanMask)
                return al::make_optional("Invalid chan_mask: "+std::to_string(ChanMask));
        }
        else if(command == "/dec/freq_bands")
        {
            if(FreqBands)
                return al::make_optional<std::string>("Duplicate freq_bands");

            istr >> FreqBands;
            if(!istr.eof() && !std::isspace(istr.peek()))
                return al::make_optional("Extra junk after freq_bands: " +
                    buffer.substr(static_cast<std::size_t>(istr.tellg())));

            if(FreqBands != 1 && FreqBands != 2)
                return al::make_optional("Invalid freq_bands: "+std::to_string(FreqBands));
        }
        else if(command == "/dec/speakers")
        {
            if(NumSpeakers)
                return al::make_optional<std::string>("Duplicate speakers");

            istr >> NumSpeakers;
            if(!istr.eof() && !std::isspace(istr.peek()))
                return al::make_optional("Extra junk after speakers: " +
                    buffer.substr(static_cast<std::size_t>(istr.tellg())));

            if(!NumSpeakers)
                return al::make_optional("Invalid speakers: "+std::to_string(NumSpeakers));
            Speakers = std::make_unique<SpeakerConf[]>(NumSpeakers);
        }
        else if(command == "/dec/coeff_scale")
        {
            std::string scale = read_word(istr);
            if(scale == "n3d") CoeffScale = AmbDecScale::N3D;
            else if(scale == "sn3d") CoeffScale = AmbDecScale::SN3D;
            else if(scale == "fuma") CoeffScale = AmbDecScale::FuMa;
            else
                return al::make_optional("Unexpected coeff_scale: "+scale);
        }
        else if(command == "/opt/xover_freq")
        {
            istr >> XOverFreq;
            if(!istr.eof() && !std::isspace(istr.peek()))
                return al::make_optional("Extra junk after xover_freq: " +
                    buffer.substr(static_cast<std::size_t>(istr.tellg())));
        }
        else if(command == "/opt/xover_ratio")
        {
            istr >> XOverRatio;
            if(!istr.eof() && !std::isspace(istr.peek()))
                return al::make_optional("Extra junk after xover_ratio: " +
                    buffer.substr(static_cast<std::size_t>(istr.tellg())));
        }
        else if(command == "/opt/input_scale" || command == "/opt/nfeff_comp" ||
                command == "/opt/delay_comp" || command == "/opt/level_comp")
        {
            /* Unused */
            read_word(istr);
        }
        else if(command == "/speakers/{")
        {
            if(!NumSpeakers)
                return al::make_optional<std::string>("Speakers defined without a count");

            const auto endpos = static_cast<std::size_t>(istr.tellg());
            if(!is_at_end(buffer, endpos))
                return al::make_optional("Extra junk on line: " + buffer.substr(endpos));
            buffer.clear();

            if(auto err = load_ambdec_speakers(Speakers.get(), NumSpeakers, f, buffer))
                return err;
            speakers_loaded = true;

            if(!read_clipped_line(f, buffer))
                return al::make_optional<std::string>("Unexpected end of file");
            std::istringstream istr2{buffer};
            std::string endmark{read_word(istr2)};
            if(endmark != "/}")
                return al::make_optional("Expected /} after speaker definitions, got "+endmark);
            istr.swap(istr2);
        }
        else if(command == "/lfmatrix/{" || command == "/hfmatrix/{" || command == "/matrix/{")
        {
            if(!NumSpeakers)
                return al::make_optional<std::string>("Matrix defined without a count");
            const auto endpos = static_cast<std::size_t>(istr.tellg());
            if(!is_at_end(buffer, endpos))
                return al::make_optional("Extra junk on line: " + buffer.substr(endpos));
            buffer.clear();

            if(!Matrix)
            {
                Matrix = std::make_unique<CoeffArray[]>(NumSpeakers * FreqBands);
                LFMatrix = Matrix.get();
                HFMatrix = LFMatrix + NumSpeakers*(FreqBands-1);
            }

            if(FreqBands == 1)
            {
                if(command != "/matrix/{")
                    return al::make_optional(
                        "Unexpected \""+command+"\" type for a single-band decoder");
                if(auto err = load_ambdec_matrix(HFOrderGain, HFMatrix, NumSpeakers, f, buffer))
                    return err;
                matrix_loaded = true;
            }
            else
            {
                if(command == "/lfmatrix/{")
                {
                    if(auto err=load_ambdec_matrix(LFOrderGain, LFMatrix, NumSpeakers, f, buffer))
                        return err;
                    lfmatrix_loaded = true;
                }
                else if(command == "/hfmatrix/{")
                {
                    if(auto err=load_ambdec_matrix(HFOrderGain, HFMatrix, NumSpeakers, f, buffer))
                        return err;
                    matrix_loaded = true;
                }
                else
                    return al::make_optional(
                        "Unexpected \""+command+"\" type for a dual-band decoder");
            }

            if(!read_clipped_line(f, buffer))
                return al::make_optional<std::string>("Unexpected end of file");
            std::istringstream istr2{buffer};
            std::string endmark{read_word(istr2)};
            if(endmark != "/}")
                return al::make_optional("Expected /} after matrix definitions, got "+endmark);
            istr.swap(istr2);
        }
        else if(command == "/end")
        {
            const auto endpos = static_cast<std::size_t>(istr.tellg());
            if(!is_at_end(buffer, endpos))
                return al::make_optional("Extra junk on end: " + buffer.substr(endpos));

            if(!speakers_loaded || !matrix_loaded || (FreqBands == 2 && !lfmatrix_loaded))
                return al::make_optional<std::string>("No decoder defined");

            return al::nullopt;
        }
        else
            return al::make_optional("Unexpected command: " + command);

        istr.clear();
        const auto endpos = static_cast<std::size_t>(istr.tellg());
        if(!is_at_end(buffer, endpos))
            return al::make_optional("Extra junk on line: " + buffer.substr(endpos));
        buffer.clear();
    }
    return al::make_optional<std::string>("Unexpected end of file");
}
