# Localization foundation

The localization catalog is loaded from `data/_ui/localization.txt` in every
active resource source. Sources are processed in the same order as the normal
game data, so a plugin can override a catalog entry from an earlier source.

Catalog entries use this format:

```
language "en"
	translation "ui.main.quit" "_Quit"
language "ko"
	translation "ui.main.quit" "_종료"
```

The language is read from `preferences.txt`:

```
language "ko"
```

Regional language tags such as `ko-KR` first look for an exact catalog and then
fall back to `ko`. A missing translation falls back to English, and a missing
English entry returns the key so an incomplete catalog is visible during
development.

Static interface text opts in with an `@` prefix, for example:

```
button q @ui.main.quit
```

This first foundation migrates representative main-menu labels and includes a
Korean catalog smoke test. The existing bitmap font is still ASCII-only; a
Unicode font and shaping/line-breaking follow-up is required before Korean
text can be rendered correctly in-game.
