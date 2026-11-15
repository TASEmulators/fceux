#!/bin/sh
# Creates or updates src/translations/fceux_<locale>.ts from the translatable
# strings in the source tree. Translate the result with Qt Linguist; the build
# compiles every fceux_<locale>.ts into the executable automatically.
#
# Usage: scripts/updateTranslations.sh <locale>      (e.g. de, pt_BR)
#
# Set LUPDATE to override the lupdate executable (e.g. lupdate-qt5).

set -e

if [ $# -ne 1 ]; then
	echo "Usage: $0 <locale>   (e.g. de, pt_BR)" >&2
	exit 1
fi

ROOT=$(cd "$(dirname "$0")/.." && pwd)
OUT="$ROOT/src/translations/fceux_$1.ts"
LUPDATE=${LUPDATE:-lupdate}

mkdir -p "$ROOT/src/translations"
cd "$ROOT/src/translations"

"$LUPDATE" ../drivers/Qt -locations relative -no-obsolete -ts "$OUT"
