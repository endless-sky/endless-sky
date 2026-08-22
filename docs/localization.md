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

The renderer keeps the existing bitmap font for ASCII text and loads
`images/font/NotoSansKR-VF.ttf` as a Unicode glyph fallback. Korean catalog
entries can therefore be rendered in-game. The fallback currently provides
glyph lookup and basic metrics; language-specific shaping remains a separate
follow-up for scripts that require it.
