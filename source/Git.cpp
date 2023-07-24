/* Git.cpp
Copyright (c) 2023 by RisingLeaf(https://github.com/RisingLeaf)

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Git.h"

#include <git2/commit.h>
#include <git2/errors.h>
#include <git2/indexer.h>
#include <git2/types.h>
#include <iostream>

namespace {
	bool initialized = false;
	const char *fetch_url;
	const char *fetch_ref;
};



int Git::Clone(const char *url, const char *path)
{
	if(!initialized)
		git_libgit2_init();

	git_clone_options clone_opts = GIT_CLONE_OPTIONS_INIT;

	clone_opts.checkout_opts.checkout_strategy = GIT_CHECKOUT_SAFE;
	clone_opts.fetch_opts.callbacks.transfer_progress = fetch_progress;

	git_repository *repo = NULL;
	int error = git_clone(&repo, url, path, &clone_opts);
	if(error != 0)
		std::cout<<git_error_last()->message<<std::endl;
	return error;
}



int Git::Pull(const char *path)
{
	if(!initialized)
		git_libgit2_init();

	git_repository *repo = NULL;
	git_remote *remote;

	int error = git_repository_open(&repo, path);
	if(error != 0)
	{
		ErrorHandle(error);
		return error;
	}

	error = git_remote_lookup(&remote, repo, "origin");
	if(error != 0)
	{
		ErrorHandle(error);
		return error;
	}

	error = git_remote_fetch(remote, NULL, NULL, NULL);
	if(error != 0)
	{
		ErrorHandle(error);
		return error;
	}

	git_oid id_to_merge;
	git_repository_fetchhead_foreach(repo, fetchhead_cb, &id_to_merge);

	git_annotated_commit *commit;
	error = git_annotated_commit_from_fetchhead(&commit, repo, "master", fetch_url, &id_to_merge);
	if(error != 0)
	{
		ErrorHandle(error);
		return error;
	}

	const git_annotated_commit *ccommit = commit;
	error = git_merge(repo, &ccommit, 1, NULL, NULL);
	if(error != 0)
	{
		ErrorHandle(error);
		return error;
	}

	return error;
}



int Git::fetch_progress(const git_indexer_progress *stats, void *payload)
{
	int fetch_percent = (100 * stats->received_objects) / stats->total_objects;
	int index_percent = (100 * stats->indexed_objects) / stats->total_objects;
	int kbytes = stats->received_bytes / 1024;

	printf("network %3d%% (%4d kb, %5d/%5d)  /  index %3d%% (%5d/%5d)\n", fetch_percent, kbytes,
		stats->received_objects, stats->total_objects, index_percent, stats->indexed_objects,
		stats->total_objects);
	
	return 0;
}



int Git::fetchhead_cb(const char *ref_name, const char *remote_url, const git_oid *oid, unsigned int is_merge, void *payload)
{
    if (is_merge)
    {
        printf("reference: '%s' is the reference we should merge\n", ref_name);
        git_oid_cpy((git_oid *)payload, oid);
		fetch_url = remote_url;
		fetch_ref = ref_name;
    }

	return 0;
}



void Git::ErrorHandle(int error)
{
	std::cout<<git_error_last()->message<<std::endl;
}