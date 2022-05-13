#if defined(__clang__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wgnu-zero-variadic-macro-arguments"
#elif defined(__GNUC__) || defined(__GNUG__)
// Can't turn off the variadic macro warning emitted from -pedantic
#  pragma GCC system_header
#elif defined(_MSC_VER)
// Doesn't seem to emit the warning
#else
// Don't know the compiler... just let it go through
#endif


#define sandbox_fields_reflection_mylib_class_TIFFClientInfoLink(f, g, ...)    \
  f(struct client_info* , next            , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(void*               , data            , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(char*               , name            , FIELD_NORMAL, ##__VA_ARGS__) g()

#define sandbox_fields_reflection_mylib_class_TIFFTagMethods(f, g, ...)    \
  f(TIFFVSetMethod      , vsetfield       , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(TIFFVGetMethod      , vgetfield       , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(TIFFPrintMethod     , printdir        , FIELD_NORMAL, ##__VA_ARGS__) g()

#define sandbox_fields_reflection_mylib_class_TIFFFieldArray(f, g, ...)    \
  f(TIFFFieldArrayType  , type            , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint32              , allocated_size  , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint32              , count           , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(TIFFField*          , fields          , FIELD_NORMAL, ##__VA_ARGS__) g()

#define sandbox_fields_reflection_mylib_class_TIFFField(f, g, ...)    \
  f(uint32              , field_tag       , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(short               , field_readcount , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(short               , field_writecount, FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(TIFFDataType        , field_type      , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint32              , reserved        , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(TIFFSetGetFieldType , set_field_type  , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(TIFFSetGetFieldType , get_field_type  , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(unsigned short      , field_bit       , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(unsigned char       , field_oktochange, FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(unsigned char       , field_passcount , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(char*               , field_name      , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(TIFFFieldArray*     , field_subfields , FIELD_NORMAL, ##__VA_ARGS__) g()

#define sandbox_fields_reflection_mylib_class_TIFFTagValue(f, g, ...)    \
  f(const TIFFField* , info          , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(int              , count         , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(void *           , value         , FIELD_NORMAL, ##__VA_ARGS__) g() \

#define sandbox_fields_reflection_mylib_class_TIFFDirEntry(f, g, ...)    \
  f(uint16           , tdir_tag      , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint16           , tdir_type     , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint64           , tdir_count    , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(union            , tdir_offset   , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint8            , tdir_ignore   , FIELD_NORMAL, ##__VA_ARGS__) g()

#define sandbox_fields_reflection_mylib_class_TIFFDirectory(f, g, ...)    \
  f(unsigned long [4], td_fieldsset               , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint32           , td_imagewidth              , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint32           , td_imagelength             , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint32           , td_imagedepth              , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint32           , td_tilewidth               , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint32           , td_tilelength              , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint32           , td_tiledepth               , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint32           , td_subfiletype             , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint16           , td_bitspersample           , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint16           , td_sampleformat            , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint16           , td_compression             , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint16           , td_photometric             , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint16           , td_threshholding           , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint16           , td_fillorder               , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint16           , td_orientation             , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint16           , td_samplesperpixel         , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint32           , td_rowsperstrip            , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint16           , td_minsamplevalue          , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint16           , td_maxsamplevalue          , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(double*          , td_sminsamplevalue         , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(double*          , td_smaxsamplevalue         , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(float            , td_xresolution             , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(float            , td_yresolution             , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint16           , td_resolutionunit          , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint16           , td_planarconfig            , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(float            , td_xposition               , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(float            , td_yposition               , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint16 [2]       , td_pagenumber              , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint16* [3]      , td_colormap                , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint16 [2]       , td_halftonehints           , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint16           , td_extrasamples            , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint16*          , td_sampleinfo              , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint32           , td_stripsperimage          , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint32           , td_nstrips                 , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint64*          , td_stripoffset_p           , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint64*          , td_stripbytecount_p        , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint32           , td_stripoffsetbyteallocsize, FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(int              , td_stripbytecountsorted    , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(TIFFDirEntry     , td_stripoffset_entry       , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(TIFFDirEntry     , td_stripbytecount_entry    , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint16           , td_nsubifd                 , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint64*          , td_subifd                  , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint16 [2]       , td_ycbcrsubsampling        , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint16           , td_ycbcrpositioning        , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint16* [3]      , td_transferfunction        , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(float*           , td_refblackwhite           , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(int              , td_inknameslen             , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(char*            , td_inknames                , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(int              , td_customValueCount        , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(TIFFTagValue*    , td_customValues            , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(unsigned char    , td_deferstrilearraywriting , FIELD_NORMAL, ##__VA_ARGS__) g() \

#define sandbox_fields_reflection_mylib_class_Tiff(f, g, ...)    \
  f(char *           , tif_name         , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(int              , tif_fd           , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(int              , tif_mode         , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint32           , tif_flags        , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint64           , tif_diroff       , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint64           , tif_nextdiroff   , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint64*          , tif_dirlist      , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint16           , tif_dirlistsize  , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint16           , tif_dirnumber    , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(TIFFDirectory    , tif_dir          , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(TIFFDirectory    , tif_customdir    , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(union            , tif_header       , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint16           , tif_header_size  , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint32           , tif_row          , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint16           , tif_curdir       , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint32           , tif_curstrip     , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint64           , tif_curoff       , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint64           , tif_dataoff      , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint16           , tif_nsubifd      , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint64           , tif_subifdoff    , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint32           , tif_col          , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint32           , tif_curtile      , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(tmsize_t         , tif_tilesize     , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(int              , tif_decodestatus , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(TIFFBoolMethod   , tif_fixuptags    , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(TIFFBoolMethod   , tif_setupdecode  , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(TIFFPreMethod    , tif_predecode    , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(TIFFBoolMethod   , tif_setupencode  , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(int              , tif_encodestatus , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(TIFFPreMethod    , tif_preencode    , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(TIFFBoolMethod   , tif_postencode   , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(TIFFCodeMethod   , tif_decoderow    , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(TIFFCodeMethod   , tif_encoderow    , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(TIFFCodeMethod   , tif_decodestrip  , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(TIFFCodeMethod   , tif_encodestrip  , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(TIFFCodeMethod   , tif_decodetile   , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(TIFFCodeMethod   , tif_encodetile   , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(TIFFVoidMethod   , tif_close        , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(TIFFSeekMethod   , tif_seek         , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(TIFFVoidMethod   , tif_cleanup      , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(TIFFStripMethod  , tif_defstripsize , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(TIFFTileMethod   , tif_deftilesize  , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint8*           , tif_data         , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(tmsize_t         , tif_scanlinesize , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(tmsize_t         , tif_scanlineskew , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint8*           , tif_rawdata      , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(tmsize_t         , tif_rawdatasize  , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(tmsize_t         , tif_rawdataoff   , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(tmsize_t         , tif_rawdataloaded, FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint8*           , tif_rawcp        , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(tmsize_t         , tif_rawcc        , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(uint8*           , tif_base         , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(tmsize_t         , tif_size         , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(TIFFMapFileProc  , tif_mapproc      , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(TIFFUnMapFileProc, tif_unmapproc    , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(thandle_t        , tif_clientdata   , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(TIFFReadWriteProc, tif_readproc     , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(TIFFReadWriteProc, tif_writeproc    , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(TIFFSeekProc     , tif_seekproc     , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(TIFFCloseProc    , tif_closeproc    , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(TIFFSizeProc     , tif_sizeproc     , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(TIFFPostMethod   , tif_postdecode   , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(TIFFField*       , tif_fields       , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(size_t           , tif_nfields      , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(const TIFFField* , tif_foundfield   , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(TIFFTagMethods   , tif_tagmethods   , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(TIFFClientInfoLink*, tif_clientinfo , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(TIFFFieldArray  *, tif_fieldscompat , FIELD_NORMAL, ##__VA_ARGS__) g() \
  f(size_t           , tif_nfieldscompat, FIELD_NORMAL, ##__VA_ARGS__) g()

#define sandbox_fields_reflection_mylib_allClasses(f, ...)  \
  f(Tiff, mylib, ##__VA_ARGS__)                            \
  f(TiffDirectory, mylib, ##__VA_ARGS__)                   \
  f(TiffTagValue, mylib, ##__VA_ARGS__)                    \
  f(TiffField, mylib, ##__VA_ARGS__)                       \
  f(TiffFieldArray, mylib, ##__VA_ARGS__)                  \
  f(TiffTagMethods, mylib, ##__VA_ARGS__)                  \
  f(TiffClientInfoLink, mylib, ##__VA_ARGS__)              \
  f(TiffFieldArray, mylib, ##__VA_ARGS__)                      


#if defined(__clang__)
#  pragma clang diagnostic pop
#elif defined(__GNUC__) || defined(__GNUG__)
#elif defined(_MSC_VER)
#else
#endif