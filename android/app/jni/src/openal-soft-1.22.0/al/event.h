#ifndef AL_EVENT_H
#define AL_EVENT_H

struct ALCcontext;

void StartEventThrd(ALCcontext *ctx);
void StopEventThrd(ALCcontext *ctx);

#endif
