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

#include <git2/errors.h>
#include <git2/indexer.h>
#include <iostream>

namespace {
	bool initialized = false;
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
	git_libgit2_init();

	git_repository *repo = NULL;
	git_remote *remote;

	int error = git_repository_open(&repo, path);
	if(error != 0)
	{
		std::cout<<git_error_last()->message<<std::endl;
		return error;
	}

	error = git_remote_lookup(&remote, repo, "origin");
	if(error != 0)
	{
		std::cout<<git_error_last()->message<<std::endl;
		return error;
	}

	git_fetch_options options = GIT_FETCH_OPTIONS_INIT;
	options.callbacks.transfer_progress = fetch_progress;
	error = git_remote_fetch(remote, NULL, &options, NULL);
	if(error != 0)
	{
		std::cout<<git_error_last()->message<<std::endl;
		return error;
	}

	git_repository_fetchhead_foreach(repo, NULL, NULL);

	git_merge_options merge_options = GIT_MERGE_OPTIONS_INIT;
	git_checkout_options checkout_options = GIT_CHECKOUT_OPTIONS_INIT;
	checkout_options.checkout_strategy = GIT_CHECKOUT_SAFE;
	git_annotated_commit *heads[1];
	git_reference *ref;

	error = git_annotated_commit_lookup( &heads[ 0 ], repo, &branchOidToMerge );
   	if(error != 0)
	{
		std::cout<<git_error_last()->message<<std::endl;
		return error;
	}
	error = git_merge(repo, (const git_annotated_commit **)heads, 1, &merge_options, &checkout_options);
   	if(error != 0)
	{
		std::cout<<git_error_last()->message<<std::endl;
		return error;
	}
	git_annotated_commit_free(heads[ 0 ]);
	git_repository_state_cleanup(repo);

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