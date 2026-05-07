Commit current changes following project conventions.

## Rules

1. Use `git commit -sS` (Signed-off-by + GPG signature)
2. Follow conventional commits format: `type(scope): description`
   - Types: `feat`, `fix`, `refactor`, `docs`, `test`, `ci`, `build`, `deps`, `chore`
   - Scopes used in this project: `api`, `parser`, `elaborator`, `csv2rdl`, `render`,
     `template`, `register`, `systemrdl`, `cmdline`, `validation`, `json_validator`,
     `template_render_validator`, `RCSV`, `workflows`, `release`, `ci`, `test`, `deps`
3. Body uses bullet points (`-`), 1-3 bullets only (5 max if pieces are truly independent)
4. Do NOT include any AI information (no `Co-Authored-By: Claude`, no AI tool mentions)
5. Commit message must be in English

## Voice

Plain English, ruthlessly concise. The diff carries the implementation
details; the message carries the intent. If one sentence works, do not
write two.

- Title: under 72 chars, imperative mood, lowercase first word, what changed (not how)
- Body: 1-3 short bullets typically, max 5 if pieces are truly independent
- Total: aim for 5-8 lines including title, blank lines, and Signed-off-by
- Reduction test: after writing each bullet, ask "could I delete this
  without losing meaning?" If yes, delete

Avoid: per-method bullets, restating the title, paragraph essays,
numbered step-by-step flows, API signatures dumped into the body.

## Steps

1. Run `git status` and `git diff --cached` to understand staged changes
2. Run `git log --oneline -10` to check recent commit style
3. Draft a commit message matching the project style
4. Run `git commit -sS -m "<message>"` using HEREDOC for multi-line messages

## Examples

Single-line (small fix):

```
git commit -sS -m "fix(api): capture ANTLR4 errors and silence library output

Signed-off-by: Huang Rui <vowstar@gmail.com>"
```

Multi-bullet (cross-platform feature):

```
git commit -sS -m "$(cat <<'EOF'
ci(release): add three-platform release workflow

- Linux on AlmaLinux 8 with bundled libstdc++/libgcc and patchelf rpath
- Windows on MSYS2 mingw64 with ANTLR4 and MinGW runtime DLLs bundled
- macOS arm64 with @rpath/@loader_path fixups

Signed-off-by: Huang Rui <vowstar@gmail.com>
EOF
)"
```

Refactor:

```
git commit -sS -m "$(cat <<'EOF'
refactor(validation): simplify row type validation logic

- Collapse redundant branches into a single dispatch table
- Remove unused field-type fallback path

Signed-off-by: Huang Rui <vowstar@gmail.com>
EOF
)"
```
