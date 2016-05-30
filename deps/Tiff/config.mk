PRJTYPE = StaticLib
SRC = \
	src/tif_aux.c \
	src/tif_close.c \
	src/tif_codec.c \
	src/tif_color.c \
	src/tif_compress.c \
	src/tif_dir.c \
	src/tif_dirinfo.c \
	src/tif_dirread.c \
	src/tif_dirwrite.c \
	src/tif_dumpmode.c \
	src/tif_error.c \
	src/tif_extension.c \
	src/tif_fax3.c \
	src/tif_fax3sm.c \
	src/tif_flush.c \
	src/tif_getimage.c \
	src/tif_jbig.c \
	src/tif_jpeg.c \
	src/tif_jpeg_12.c \
	src/tif_luv.c \
	src/tif_lzma.c \
	src/tif_lzw.c \
	src/tif_next.c \
	src/tif_ojpeg.c \
	src/tif_open.c \
	src/tif_packbits.c \
	src/tif_pixarlog.c \
	src/tif_predict.c \
	src/tif_print.c \
	src/tif_read.c \
	src/tif_strip.c \
	src/tif_swab.c \
	src/tif_thunder.c \
	src/tif_tile.c \
	src/tif_version.c \
	src/tif_warning.c \
	src/tif_write.c \
	src/tif_zip.c \
	src/tif_stream.cxx
ifeq ($(OS), Windows_NT)
	SRC += src/tif_win32.c
else
	SRC += src/tif_unix.c
endif
