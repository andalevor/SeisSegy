# SeisSegy
SEGY r/w library

# Build
You will need SeisTrace library.

1) git clone --recurse-submodules https://github.com/andalevor/SeisSegy.git

2) cd SeisSegy

3) meson setup -Dbuildtype=release -Db_ndebug=true build 

or

3) meson setup -Dbuildtype=release -Db_ndebug=true -Dc_args=-DSU_BIG_ENDIAN build

to read/write SU files always in big endian

4) meson compile -C build seissegy

5) meson install -C build
