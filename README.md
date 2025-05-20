# SeisSegy
SEGY r/w library

# Build
You will need SeisTrace library.

git clone --recurse-submodules https://github.com/andalevor/SeisSegy.git

cd SeisSegy

meson setup -Dbuildtype=release build
or
meson setup -Dbuildtype=release -Dc_args=-DSU_BIG_ENDIAN build
to read/write SU files always in big endian

meson compile -C build seissegy

meson install -C build
