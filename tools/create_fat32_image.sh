#!/usr/bin/env bash
set -euo pipefail

IMG_PATH="${1:-build/disk.img}"
TMP_DIR="$(mktemp -d)"
cleanup() { rm -rf "${TMP_DIR}"; }
trap cleanup EXIT

mkdir -p "$(dirname "${IMG_PATH}")"
truncate -s 64M "${IMG_PATH}"

if command -v parted >/dev/null 2>&1; then
  parted -s "${IMG_PATH}" mklabel msdos
  parted -s "${IMG_PATH}" mkpart primary fat32 1MiB 100%
  parted -s "${IMG_PATH}" set 1 boot on
else
  echo "[ERRO] parted nao encontrado."
  exit 1
fi

if ! command -v mformat >/dev/null 2>&1 || ! command -v mcopy >/dev/null 2>&1; then
  echo "[ERRO] mtools (mformat/mcopy) nao encontrado."
  exit 1
fi

cat > "${TMP_DIR}/README.TXT" <<'EOF'
hello from nanaos disk
EOF

cp userspace/hello/hello.elf "${TMP_DIR}/HELLO.ELF"

# offset 1MiB => 2048 setores de 512 bytes
mformat -i "${IMG_PATH}"@@1048576 -F ::
mcopy -i "${IMG_PATH}"@@1048576 "${TMP_DIR}/README.TXT" ::
mcopy -i "${IMG_PATH}"@@1048576 "${TMP_DIR}/HELLO.ELF" ::

echo "[OK] FAT32 image criada em ${IMG_PATH}"
