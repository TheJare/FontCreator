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

#ifndef _FONT_GENERATOR_H_
#define _FONT_GENERATOR_H_

#include <vector>

// -------------------------------------------------------------------
// Crappy Font structure
struct BmFont
{
  struct BmTex
  {
    unsigned w, h;
    std::vector<unsigned char> pix;

    BmTex(): w(0), h(0), pix() { }
    BmTex(unsigned _w, unsigned _h): w(_w), h(_h), pix() { pix.resize(_w*_h); }
    ~BmTex() { }

    void Resize(unsigned _w, unsigned _h){ w = _w; h = _h; pix.resize(_w*_h); }

    unsigned char &at(unsigned x, unsigned y) { return pix[x+y*w]; }
    const unsigned char &at(unsigned x, unsigned y) const { return pix[x+y*w]; }

  };
  struct BmChar
  {
    unsigned ch;
    unsigned tx;
    unsigned txu, txv;
    unsigned w, h;
    int      offx, offy; // Add to pen coords to determine where to draw the actual bitmap
    int      advx;

    BmTex    Pix;

    BmChar(): ch(0), tx(0), txu(0), txv(0), w(0), h(0), offx(0), offy(0), advx(0) { }
    void Resize(unsigned _w, unsigned _h){ Pix.Resize(_w, _h); w = _w; h = _h; }
  };

  std::vector<BmChar> Chars;
  std::vector<BmTex>  Textures;
  int height;
//  int ascender;
//  int descender;
};

// -------------------------------------------------------------------
void GenerateFont(BmFont &Font, const char *font, int dpi, int size, int boldf, int italicf, int texsize, const std::vector<wchar_t> &charset);

#endif // _FONT_GENERATOR_H_
