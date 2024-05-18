//20180318
#pragma once

extern int PyCodec_Register(PyObject *search_function);
extern PyObject * _PyCodec_Lookup(const char *encoding);
extern PyObject * PyCodec_Encode(PyObject *object, const char *encoding, const char *errors);
extern PyObject * PyCodec_Decode(PyObject *object, const char *encoding, const char *errors);
extern PyObject * PyCodec_Encoder(const char *encoding);
extern PyObject * PyCodec_Decoder(const char *encoding);
extern PyObject * PyCodec_StreamReader(const char *encoding, PyObject *stream, const char *errors);
extern PyObject * PyCodec_StreamWriter(const char *encoding, PyObject *stream, const char *errors);
