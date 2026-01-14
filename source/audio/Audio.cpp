/* Audio.cpp
Copyright (c) 2014 by Michael Zahniser

Endless Sky is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later version.

Endless Sky is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include "Audio.h"

#include "player/AudioPlayer.h"
#include "supplier/effect/Fade.h"
#include "../Files.h"
#include "../Logger.h"
#include "Music.h"
#include "player/MusicPlayer.h"
#include "../Point.h"
#include "Sound.h"

#include <AL/al.h>
#include <AL/alc.h>

#include <algorithm>
#include <cmath>
#include <map>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <vector>

using namespace std;

namespace {
	// This class represents a new sound source that is queued to be added. This
	// is so that any thread can add a sound, but the audio thread can control
	// when those sounds actually start playing.
	class QueueEntry {
	public:
		void Add(Point position, SoundCategory category);
		void Add(const QueueEntry &other);

		Point sum;
		double weight = 0.;
		SoundCategory category = SoundCategory::MASTER;
	};

	void Move(const AudioPlayer &player, QueueEntry entry)
	{
		Point angle = entry.sum / entry.weight;
		// The source should be along the vector (angle.X(), angle.Y(), -1).
		// The length of the vector should be sqrt(1 / weight).
		double scale = sqrt(1. / (entry.weight * (angle.LengthSquared() + 1.)));
		player.Move(angle.X() * scale, angle.Y() * scale, -scale);
	}

	// Thread entry point for loading the sound files.
	void Load();


	// Mutex to make sure different threads don't modify the audio at the same time.
	mutex audioMutex;

	// OpenAL settings.
	ALCdevice *device = nullptr;
	ALCcontext *context = nullptr;
	bool isInitialized = false;

	// We keep track of the volume levels requested, and the volume levels
	// currently set in OpenAL.
	map<SoundCategory, double> volume{{SoundCategory::MASTER, .125}};
	map<SoundCategory, double> cachedVolume;

	// This queue keeps track of sounds that have been requested to play. Each
	// added sound is "deferred" until the next audio position update to make
	// sure that all sounds from a given frame start at the same time.
	map<const Sound *, QueueEntry> soundQueue;
	map<const Sound *, QueueEntry> deferred;
	thread::id mainThreadID;

	// Sound resources that have been loaded from files.
	map<string, Sound> sounds;

	/// The active audio sources
	vector<shared_ptr<AudioPlayer>> players;
	/// The looping players for reuse. Looping sources always have the Fade effect.
	map<const Sound *, shared_ptr<AudioPlayer>> loopingPlayers;

	// Queue and thread for loading sound files in the background.
	map<string, filesystem::path> loadQueue;
	thread loadThread;

	// The current position of the "listener," i.e. the center of the screen.
	Point listener;

	// The active music player, if there is one. This player is also present in 'player'.
	// In the current implementation, the supplier of the music source is always a Fade.
	shared_ptr<AudioPlayer> musicPlayer;
	string currentTrack;

	// The number of Pause vs Resume requests received.
	int pauseChangeCount = 0;
	// If we paused the audio multiple times, only resume it after the same number of Resume() calls.
	// We start with -1, so when MenuPanel opens up the first time, it doesn't pause the loading sounds.
	int pauseCount = -1;
	// "Audio::Pause" and "Audio::Resume" have no effect when this is "true",
	// so a panel can prevent others appearing on top of it from pausing its sounds.
	bool pausingBlocked = false;
}



// Begin loading sounds (in a separate thread).
void Audio::Init(const vector<filesystem::path> &sources)
{
	device = alcOpenDevice(nullptr);
	if(!device)
		return;

	context = alcCreateContext(device, nullptr);
	if(!context || !alcMakeContextCurrent(context))
		return;

	// If we don't make it to this point, no audio will be played.
	isInitialized = true;
	mainThreadID = this_thread::get_id();

	// The listener is looking "into" the screen. This orientation vector is
	// used to determine what sounds should be in the right or left speaker.
	ALfloat zero[3] = {0., 0., 0.};
	ALfloat orientation[6] = {0., 0., -1., 0., 1., 0.};

	alListenerf(AL_GAIN, volume[SoundCategory::MASTER]);
	alListenerfv(AL_POSITION, zero);
	alListenerfv(AL_VELOCITY, zero);
	alListenerfv(AL_ORIENTATION, orientation);
	alDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
	alDopplerFactor(0.);

	LoadSounds(sources);
}



// Get all the sound files in the game data and all plugins.
void Audio::LoadSounds(const vector<filesystem::path> &sources)
{
	for(const auto &source : sources)
	{
		filesystem::path root = source / "sounds";
		vector<filesystem::path> files = Files::RecursiveList(root);
		for(const auto &path : files)
		{
			if(path.extension() == ".wav")
			{
				// The "name" of the sound is its full path within the "sounds/"
				// folder, without the ".wav" or "~.wav" suffix.
				string name = (path.parent_path() / path.stem()).lexically_relative(root).generic_string();
				if(name.ends_with('~'))
					name.resize(name.length() - 1);
				loadQueue[name] = path;
			}
		}
	}
	// Begin loading the files.
	if(!loadQueue.empty())
		loadThread = thread(&Load);
}



void Audio::CheckReferences(bool parseOnly)
{
	if(!isInitialized && !parseOnly)
	{
		Logger::Log("Audio could not be initialized. No audio will play.", Logger::Level::WARNING);
		return;
	}

	for(auto &&it : sounds)
		if(it.second.Name().empty())
			Logger::Log("Sound \"" + it.first + "\" is referred to, but does not exist.", Logger::Level::WARNING);
}



// Report the progress of loading sounds.
double Audio::GetProgress()
{
	unique_lock<mutex> lock(audioMutex);

	if(loadQueue.empty())
		return 1.;

	double done = sounds.size();
	double total = done + loadQueue.size();
	return done / total;
}



// Get the volume.
double Audio::Volume(SoundCategory category)
{
	if(!volume.contains(category))
		volume[category] = 1.;

	return volume[category];
}



// Set the volume (to a value between 0 and 1).
void Audio::SetVolume(double level, SoundCategory category)
{
	volume[category] = clamp(level, 0., 1.);
}



// Get a pointer to the named sound. The name is the path relative to the
// "sound/" folder, and without ~ if it's on the end, or the extension.
const Sound *Audio::Get(const string &name)
{
	unique_lock<mutex> lock(audioMutex);
	return &sounds[name];
}



// Set the listener's position, and also update any sounds that have been
// added but deferred because they were added from a thread other than the
// main one (the one that called Init()).
void Audio::Update(const Point &listenerPosition)
{
	if(!isInitialized)
		return;

	listener = listenerPosition;

	for(const auto &it : deferred)
		soundQueue[it.first].Add(it.second);
	deferred.clear();
}



// Play the given sound, at full volume.
void Audio::Play(const Sound *sound, SoundCategory category)
{
	Play(sound, listener, category);
}



// Play the given sound, as if it is at the given distance from the
// "listener". This will make it softer and change the left / right balance.
void Audio::Play(const Sound *sound, const Point &position, SoundCategory category)
{
	if(!isInitialized || !sound || sound->Buffer().empty() || !volume[SoundCategory::MASTER])
		return;

	// Place sounds from the main thread directly into the queue. They are from
	// the UI, and the Engine may not be running right now to call Update().
	if(this_thread::get_id() == mainThreadID)
		soundQueue[sound].Add(position - listener, category);
	else
	{
		unique_lock<mutex> lock(audioMutex);
		deferred[sound].Add(position - listener, category);
	}
}



// Play the given music. An empty string means to play nothing.
void Audio::PlayMusic(const string &name)
{
	if(!isInitialized)
		return;

	// Skip changing music if the requested music is already playing.
	if(name == currentTrack)
		return;

	// Don't worry about thread safety here, since music will always be started
	// by the main thread.
	currentTrack = name;
	if(musicPlayer && !musicPlayer->IsFinished())
		reinterpret_cast<Fade *>(musicPlayer->Supplier())->AddSource(Music::CreateSupplier(name, true));
	else
	{
		Fade *fade = new Fade();
		fade->AddSource(Music::CreateSupplier(name, true));
		musicPlayer = shared_ptr<AudioPlayer>(new MusicPlayer(unique_ptr<AudioSupplier>{fade}));
		musicPlayer->Init();
		musicPlayer->SetVolume(Volume(SoundCategory::MUSIC));
		musicPlayer->Play();
		players.emplace_back(musicPlayer);
	}
}



// Pause all active playback streams. Doesn't cause new streams to be paused, and doesn't pause the music source.
void Audio::Pause()
{
	pauseChangeCount += !pausingBlocked;
}



// Resumes all paused sound sources. If Pause() was called multiple times,
// you have to call Resume() the same number of times to resume the sound sources.
void Audio::Resume()
{
	pauseChangeCount -= !pausingBlocked;
}



void Audio::BlockPausing()
{
	pausingBlocked = true;
}



void Audio::UnblockPausing()
{
	pausingBlocked = false;
}



/// Begin playing all the sounds that have been added since the last time
/// this function was called.
/// If the game is in fast forward mode, the fast version of sounds is played.
void Audio::Step(bool isFastForward)
{
	if(!isInitialized)
		return;

	for(const auto &[category, expected] : volume)
		if(cachedVolume[category] != expected)
		{
			cachedVolume[category] = expected;
			if(category == SoundCategory::MASTER)
				alListenerf(AL_GAIN, expected);
			else
				for(const auto &player : players)
					if(player->Category() == category)
						player->SetVolume(expected);
		}

	if(pauseChangeCount > 0)
	{
		bool wasPaused = pauseCount;
		pauseCount += pauseChangeCount;
		if(pauseCount && !wasPaused)
			for(const auto &player : players)
				player->Pause();
	}
	else if(pauseChangeCount < 0)
	{
		// Check that the game is not paused after this request. Also don't allow the pause count to go into negatives.
		if(pauseCount && (pauseCount += pauseChangeCount) <= 0)
		{
			pauseCount = 0;
			for(const auto &player : players)
				player->Play();
		}
	}
	pauseChangeCount = 0;

	// For each sound that is looping, see if it is going to continue. For other
	// sounds, check if they are done playing.
	for(auto it = loopingPlayers.begin(); it != loopingPlayers.end();)
	{
		const auto &[sound, player] = *it;
		auto queueIt = soundQueue.find(sound);
		if(queueIt != soundQueue.end())
		{
			Move(*player, queueIt->second);
			soundQueue.erase(queueIt);
			++it;
		}
		else
		{
			reinterpret_cast<Fade *>(player->Supplier())->AddSource({}, 3); // fast fade
			it = loopingPlayers.erase(it);
		}
	}

	// Queue up the new buffers in every player, and remove the finished ones.
	for(const auto &player : players)
	{
		player->Supplier()->Set3x(isFastForward);
		player->Update();
	}

	erase_if(players, [](const auto &player){ return player->IsFinished(); });

	// Now, what is left in the queue is sounds that want to play, and that do
	// not correspond to an existing source.
	for(const auto &[sound, entry] : soundQueue)
	{
		unique_ptr<AudioSupplier> supplier = sound->CreateSupplier();
		supplier->Set3x(isFastForward);
		shared_ptr<AudioPlayer> player;
		if(sound->IsLooping())
		{
			unique_ptr<Fade> fade = make_unique<Fade>();
			fade->AddSource(std::move(supplier));
			player = make_shared<AudioPlayer>(entry.category, std::move(fade));
			loopingPlayers.emplace(sound, player);
		}
		else
			player = make_shared<AudioPlayer>(entry.category, std::move(supplier));

		player->Init();
		player->SetVolume(Volume(entry.category));
		Move(*player, entry);
		player->Play();

		players.emplace_back(player);
	}
	soundQueue.clear();

	if(musicPlayer && musicPlayer->IsFinished())
		musicPlayer.reset();
}



// Shut down the audio system (because we're about to quit).
void Audio::Quit()
{
	// First, check if sounds are still being loaded in a separate thread, and
	// if so interrupt that thread and wait for it to quit.
	unique_lock<mutex> lock(audioMutex);
	if(!loadQueue.empty())
		loadQueue.clear();
	if(loadThread.joinable())
	{
		lock.unlock();
		loadThread.join();
		lock.lock();
	}

	// Now, stop and delete any OpenAL sources that are playing.
	players.clear();
	loopingPlayers.clear();
	musicPlayer.reset();

	// Free the memory buffers for all the sound resources.
	sounds.clear();

	// Close the connection to the OpenAL library.
	if(context)
	{
		alcMakeContextCurrent(nullptr);
		alcDestroyContext(context);
	}
	if(device)
		alcCloseDevice(device);
}



namespace {
	// Add a new source to this queue entry. Sources are weighted based on their
	// position, and multiple sources can be added together in the same entry.
	// The preserved category is the category of the last source.
	void QueueEntry::Add(Point position, SoundCategory category)
	{
		// A distance of 500 counts as 1 OpenAL unit of distance.
		position *= .002;
		// To avoid having sources at a distance of 0 be infinitely loud, have
		// the minimum distance be 1 unit away.
		double d = 1. / (1. + position.Dot(position));
		sum += d * position;
		weight += d;
		this->category = category;
	}

	// Combine two queue entries.
	void QueueEntry::Add(const QueueEntry &other)
	{
		sum += other.sum;
		weight += other.weight;
		category = other.category;
	}

	// Thread entry point for loading sounds.
	void Load()
	{
		string name;
		filesystem::path path;
		Sound *sound;
		while(true)
		{
			{
				unique_lock<mutex> lock(audioMutex);
				// If this is not the first time through, remove the previous item
				// in the queue. This is a signal that it has been loaded, so we
				// must not remove it until after loading the file.
				if(!path.empty() && !loadQueue.empty())
					loadQueue.erase(loadQueue.begin());
				if(loadQueue.empty())
					return;
				name = loadQueue.begin()->first;
				path = loadQueue.begin()->second;

				// @3x sounds should be merged with their regular variant here.
				if(name.ends_with("@3x"))
					name.resize(name.size() - 3);

				// Since we need to unlock the mutex below, create the map entry to
				// avoid a race condition when accessing sounds' size.
				sound = &sounds[name];
			}

			// Unlock the mutex for the time-intensive part of the loop.
			if(!sound->Load(path, name))
				Logger::Log("Unable to load sound \"" + name + "\" from path: " + path.string(),
					Logger::Level::WARNING);
		}
	}
}
