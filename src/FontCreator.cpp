/*
Copyright (C) 2006 by Javier Arévalo (http://www.iguanademos.com/Jare)

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from the use of this software.

Permission is granted to anyone to use this software for any purpose, including commercial applications,
and to alter it and redistribute it freely, subject to the following restrictions:

 1 - The origin of this software must not be misrepresented. You must not claim that you wrote
     the original software. If you use this software in a product, an acknowledgment in the product
     documentation would be appreciated but is not required.
 2 - Altered source versions must be plainly marked as such, and must not be misrepresented as being
     the original software.
 3 - This notice may not be removed or altered from any source distribution.
*/

#define  _CRT_SECURE_NO_DEPRECATE  // Evil Microsoft
#include <stdio.h>
#include <ctype.h>
#include <string>
#include <algorithm>

#include "FontGenerator.h"

// -------------------------------------------------------------------

// ---------------------------------
// Platform-independent LE writer
void WriteShort(FILE *f, unsigned short s)
{
  fputc(s & 255, f);
  fputc((s >> 8) & 255, f);
}

// ---------------------------------
// Crappy TGA writer
// comps == 1 -> 8bit grayscale TGA
// comps == 3 -> 24bit TGA
// comps == 4 -> 32bit TGA, RGB are all white and font is in the alpha channel
bool SaveTGA(const char *fname, unsigned comps, const unsigned char *p, unsigned w, unsigned h)
{
  FILE *f = fopen(fname, "wb");
  if (!f)
    return false;

  WriteShort(f, 0); // Palette entries
  fputc(comps == 1? 3 : 2, f); // 1 => paletted, 2 => RGB, 3 => grayscale
  for (unsigned i = 0; i < 9; ++i)
    fputc(0, f);
  WriteShort(f, (unsigned short)w);
  WriteShort(f, (unsigned short)h);
  WriteShort(f, (unsigned short)( (comps != 3? 0x0800 : 0) | (comps * 8)));
  for (int i = h-1; i >= 0; --i) // TGA is bottom-up
  {
    for (unsigned j = 0; j < w; ++j)
    {
      unsigned char c = p[i*w+j];
      if (comps == 1)
        fputc(c, f);
      else if (comps == 3)
      {
        fputc(c, f);
        fputc(c, f);
        fputc(c, f);
      }
      else
      {
        fputc(255, f);
        fputc(255, f);
        fputc(255, f);
        fputc(c, f);
      }
    }
  }
  fclose(f);
  return true;
}

// ---------------------------------
void SaveFont(const BmFont &Font, const char *dest, int bpp)
{
  // Save font data: description file + TGA textures
  char buf[10000];
  sprintf(buf, "%s.tfn", dest);
  FILE *f = fopen(buf, "wt");
  if (!f)
    throw std::runtime_error("Can't write to TFN file");
  fprintf(f, "%d %d %d\n", Font.Textures.size(), Font.Chars.size(), Font.height);

  // Save textures
  for (std::vector<BmFont::BmTex>::const_iterator it = Font.Textures.begin(); it != Font.Textures.end(); ++it)
  {
    char buf[10000];
    sprintf(buf, "%s_%02d.tga", dest, it - Font.Textures.begin());
    if (!SaveTGA(buf, bpp/8, (it->w && it->h)? &it->pix[0] : 0, it->w, it->h))
      throw std::runtime_error("Can't write to TGA file");

    // Write texture filename and dimensions to TFN without extension (so these can be converted to other formats later)
    sprintf(buf, "%s_%02d", dest, it - Font.Textures.begin());
    const char *p = strrchr(buf, '\\'); p = p? p+1 : buf;
    const char *q = strrchr(p, '/'); q = q? q+1 : p;
    fprintf(f, "%s\n%d %d\n", q, it->w, it->h);
  }
  for (std::vector<BmFont::BmChar>::const_iterator it = Font.Chars.begin(); it != Font.Chars.end(); ++it)
    fprintf(f, "0x%04X %d %d %d %d %d %d %d %d\n",
               it->ch,
               it->advx, it->offx, it->offy,
               it->tx, it->txu, it->txv, it->w, it->h);
  fclose(f);
}

// -------------------------------------------------------------------

// ---------------------------------
wchar_t CharToWchar(unsigned char c)
{
  wchar_t wc;
//  if (::MultiByteToWideChar(CP_ACP, 0, (LPCSTR)&c, 1, &wc, 1) != 1)
  if (mbtowc(&wc, (char*)&c, 1) <= 0)
    wc = c;
  return wc;
}

// ---------------------------------
void AddCharToCharset(std::vector<wchar_t> &charset, wchar_t c)
{
  if (c >= L' ' && std::find(charset.begin(), charset.end(), c) == charset.end())
    charset.push_back(c);
}

// ---------------------------------
void AddCharsToCharset(std::vector<wchar_t> &charset, const unsigned char *p, int size, bool checkBOF = false)
{
  if (size <= 0)
    return;

  // Check for unicode data
  bool bUnicode = false;
  bool bLEndian = false;
  if (checkBOF && size > 1)
  {
    bUnicode = (p[0] == 0xFF && p[1] == 0xFE) || (p[0] == 0xFE && p[1] == 0xFF);
    bLEndian = (p[0] == 0xFF && p[1] == 0xFE);
  }
  if (bUnicode && (size & 1) != 0)
    throw std::runtime_error("Malformed Unicode charset");

  int step = bUnicode? 2 : 1;
  for (int i = bUnicode? 2 : 0; i+step <= size; i += step) // Skip BOF if it was found
  {
    wchar_t c = 0;
    if (bUnicode)
      if (bLEndian)
        c = p[i+1]*256 + p[i+0];
      else
        c = p[i+0]*256 + p[i+1];
    else
      c = CharToWchar(p[i]);
    AddCharToCharset(charset, c);
  }
}

// ---------------------------------
void AddCharFileToCharset(std::vector<wchar_t> &charset, const char *file)
{
  FILE *f = fopen(file, "rb");
  if (!f)
    throw std::runtime_error("Unable to open charset file");
  fseek(f, 0, SEEK_END);
  long l = ftell(f);
  fseek(f, 0, SEEK_SET);
  if (l > 0)
  {
    unsigned char *p = new unsigned char[l];
    if (!p)
      throw std::runtime_error("Charset file too large");
    fread(p, l, 1, f);
    AddCharsToCharset(charset, p, (int)l, true);
    delete[] p;
  }
  fclose(f);
}

// -------------------------------------------------------------------

// ---------------------------------
void Message(const char *s)
{
  puts(s);
}

// ---------------------------------
bool IsPow2(int i)
{
  return (i & (i-1)) == 0;
}

#define APP_HELP\
  "FontCreator 1.0, Copyright (C) 2006 Javier Arevalo (http://www.iguanademos.com/Jare)\n"\
  "\n"\
  "Usage: FontCreator TTF_file size TFN_prefix [options] [charset files]\n"\
  "          (default charset is chars 32-255)\n"\
  "\n"\
  "OPTIONS:\n"\
  "\n"\
  "  -h, --help:     This help                                          \n"\
  "  -bpp #:         Bpp of bitmap files (8, 24, 32, 8 is default)      \n"\
  "  -dpi #:         DPI for font rendering (72 is default)             \n"\
  "  -bold [#]:      Generate a bold font (# is factor, default 20)     \n"\
  "  -italic [#]:    Generate an italics font (# is factor, default 20) \n"\
  "  -chars \"chars\": Add specified chars to charset                     \n"\
  "  -max texSize:   Maximum texture size (power of 2, default is 1024) \n"\
  "  -noASCII:       Do not include the 32-127 chars by default         \n"\
  "  -cp \"codepage\": Codepage for translating 8-bit characters          \n"\
  "\n"\
  "Example:\n"\
  " FontCreator C:\\Windows\\Fonts\\ARIALUNI.TTF 80 \"My Fonts\\ArialNumsIt\" -max 512 -chars \"0123456789\" -italic -bold 30\n"\
  ""

// ---------------------------------
int main(int argc, char* argv[])
{
  // Use the system codepage to convert characters to wchar_t
  setlocale(LC_ALL, "");

  std::string inf;
  std::string outf;
  int size = 0;
  int texsize = 1024;
  int boldf = 0;
  int italicf = 0;
  int bpp = 8;
  int dpi = 72;
  std::vector<wchar_t> charset;
  bool bNoASCII = false;

  bool showHelp = false;
  int nPara = 0;

  try
  {
    for (int opt = 1; opt < argc;)
    {
      _strupr(argv[opt]);
      std::string cmd = argv[opt++];

      if (cmd == "-NOASCII")
        bNoASCII = true;
      else if (cmd == "-BOLD")
      {
        boldf = 20;
        if (opt < argc && (isdigit(argv[opt][0]) || (argv[opt][0] == '-' && isdigit(argv[opt][1]))))
          boldf = atoi(argv[opt++]);
      }
      else if (cmd == "-ITALIC")
      {
        italicf = 20;
        if (opt < argc && (isdigit(argv[opt][0]) || (argv[opt][0] == '-' && isdigit(argv[opt][1]))))
          italicf = atoi(argv[opt++]);
      }
      else if (cmd == "-MAX" && opt < argc)
        texsize = atoi(argv[opt++]);
      else if (cmd == "-CP" && opt < argc)
        setlocale(LC_ALL, argv[opt++]);
      else if (cmd == "-BPP" && opt < argc)
        bpp = atoi(argv[opt++]);
      else if (cmd == "-DPI" && opt < argc)
        dpi = atoi(argv[opt++]);
      else if (cmd == "-CHARS" && opt < argc)
      {
        AddCharsToCharset(charset, (unsigned char *)argv[opt], (int)strlen(argv[opt]));
        opt++;
      }
      else if (!cmd.empty() && cmd[0] == '-')
        showHelp = true;
      else
      {
        if (nPara == 0)
          inf = cmd;
        else if (nPara == 1)
          size = atoi(cmd.c_str());
        else if (nPara == 2)
          outf = cmd;
        else
          AddCharFileToCharset(charset, cmd.c_str());
        nPara++;
      }
    }

    // Setup charset - sorted, and defaults to 32-127
    if (!bNoASCII)
      for (int i = 32; i < 127; ++i)
        AddCharToCharset(charset, CharToWchar((unsigned char)i));

    std::sort(charset.begin(), charset.end());

    // Sanity check parameters
    if (charset.empty() || showHelp || nPara < 3 || size <= 0 || texsize <= 1 || !IsPow2(texsize)
        || dpi <= 0 || (bpp != 8 && bpp != 24 && bpp != 32))
    {
      Message(APP_HELP);
      return 1;
    }

    BmFont Font;
    GenerateFont(Font, inf.c_str(), dpi, size, boldf, italicf, texsize, charset);
    SaveFont(Font, outf.c_str(), bpp);
  }
  catch (std::exception &e)
  {
    Message(APP_HELP);
    printf("\n*** ERROR:\n  %s\n", e.what());
    return 1;
  }

  return 0;
}
