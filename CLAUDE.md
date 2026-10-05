# Endless Sky: Expanded

Fork of Endless Sky adding a player industry mod. Plan, decisions and status live in
`docs/expanded/DESIGN.md` — read it before starting work, and update it at the end of a piece.

The user playtests on Windows using the build from the `playtest` release, produced by
`.github/workflows/playtest.yml` on every push to a `claude/**` branch. They have a limited
token budget: work in small, self-contained pieces and keep explanations non-technical.

## Build and check (Linux container)

Dependencies are installed by the SessionStart hook (`.claude/hooks/session-start.sh`).

```bash
cmake --preset linux -DCMAKE_COMPILE_WARNING_AS_ERROR=ON   # once; the Windows CI build uses -Werror
cmake --build --preset linux-release --target EndlessSky
xvfb-run -a ./build/linux/Release/endless-sky --parse-assets -c /tmp/escfg   # data check
python3 utils/check_code_style.py && python3 utils/check_copyright.py
```

Run one integration test (copy the config so the repo stays clean):

```bash
cp -r tests/integration/config /tmp/itest
xvfb-run -a --server-args="+extension GLX +render -noreset" \
  ./build/linux/Release/endless-sky --config /tmp/itest --test "<test name>"
```

## Conventions

- Follow upstream style (tabs, existing file header with copyright, alphabetical includes).
- New source files must be added to `source/CMakeLists.txt`.
- Keep mod content in `data/expanded/` and mod docs in `docs/expanded/` where possible,
  to keep merges from upstream easy.
