#!/usr/bin/env bash
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CHANGELOG="$REPO_ROOT/CHANGELOG.md"
DEBIAN_CHANGELOG="$REPO_ROOT/debian/changelog"
SPEC="$REPO_ROOT/packaging/rpm/whisper-echo-gtk.spec"

if [[ ! -f "$CHANGELOG" ]]; then
  echo "CHANGELOG.md not found" >&2
  exit 1
fi

# Extract latest version from CHANGELOG.md
VERSION=$(grep -E '^## \[\d+\.\d+\.\d+\]' "$CHANGELOG" | head -n1 | sed -E 's/## \[([^]]+)\].*/\1/')
if [[ -z "$VERSION" ]]; then
  echo "Could not parse version from CHANGELOG.md" >&2
  exit 1
fi

DATE=$(date -u +"%a, %d %b %Y %H:%M:%S +0000")
AUTHOR="Bryan Nielsen <bnielsen1965@gmail.com>"

# Build Debian changelog entry
DEBIAN_ENTRY="whisper-echo-gtk ($VERSION-1) unstable; urgency=medium

  * See CHANGELOG.md for details

 -- $AUTHOR  $DATE
"

# Prepend to debian/changelog if not already present
if ! grep -q "whisper-echo-gtk ($VERSION-1)" "$DEBIAN_CHANGELOG"; then
  {
    echo "$DEBIAN_ENTRY"
    cat "$DEBIAN_CHANGELOG"
  } > "$DEBIAN_CHANGELOG.tmp"
  mv "$DEBIAN_CHANGELOG.tmp" "$DEBIAN_CHANGELOG"
  echo "Updated $DEBIAN_CHANGELOG"
else
  echo "Debian changelog already contains $VERSION"
fi

# Update RPM spec changelog
if grep -q "%changelog" "$SPEC"; then
  # Insert new entry after %changelog line
  awk -v ver="$VERSION" -v date="$(date +"%a %b %d %Y")" -v author="$AUTHOR" '
    /^%changelog/ {print; print "* " date " " author " - " ver "-1"; print "- See CHANGELOG.md"; next}
    {print}
  ' "$SPEC" > "$SPEC.tmp"
  mv "$SPEC.tmp" "$SPEC"
  echo "Updated $SPEC"
else
  echo "No %changelog found in spec" >&2
fi

echo "Done. Version $VERSION"
