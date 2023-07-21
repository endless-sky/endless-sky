/* Git.h
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

#include <git2.h>



class Git
{
public:
	static int Clone(const char *url, const char *path);

private:
	static int fetch_progress(const git_indexer_progress *stats, void *payload);
};