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

#include "FontGenerator.h"

//#define  _CRT_SECURE_NO_DEPRECATE  // Evil Microsoft
//#include <stdio.h>
//#include <string>
#include <algorithm>

#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_GLYPH_H

#include "RectPlacement.h"

#pragma comment(lib, "freetype.lib")

// -------------------------------------------------------------------

// ---------------------------------
// Kindly adapted from Nehe's tutorial #43 (http://nehe.gamedev.net)
static void ProcessTTF(BmFont &Font, const char *fntName, int dpi, int size, int boldf, int italicf, const std::vector<wchar_t> &charset)
{
  Font.Chars.reserve(charset.size());

	//Create and initilize a freetype font library.
	FT_Library library;
	if (FT_Init_FreeType( &library )) 
		throw std::runtime_error("FT_Init_FreeType failed");
	FT_Face face;
	if (FT_New_Face( library, fntName, 0, &face )) 
		throw std::runtime_error("FT_New_Face failed (there is probably a problem with your font file)");
	FT_Set_Char_Size( face, size << 6, size << 6, dpi, dpi);

  Font.height = face->size->metrics.height/72;  // ?????? Whatever, this yields photoshop-compatible values

  FT_Matrix matrix = { 0x10000, 0, 0, 0x10000 };
  if (boldf == -100)
    throw std::runtime_error("Bold factor must not be -100 or font would be zero width");
  matrix.xx += boldf*0x10000/100;
  matrix.xy += italicf*0x10000/100;
  FT_Set_Transform(face, &matrix, 0);

  for (std::vector<wchar_t>::const_iterator it = charset.begin(); it != charset.end(); ++it)
  {
	  if (FT_Load_Glyph( face, FT_Get_Char_Index( face, *it ), FT_LOAD_DEFAULT ))
		  throw std::runtime_error("FT_Load_Glyph failed");
    FT_Glyph glyph;
    if (FT_Get_Glyph( face->glyph, &glyph ))
		  throw std::runtime_error("FT_Get_Glyph failed");
	  FT_Glyph_To_Bitmap( &glyph, FT_RENDER_MODE_NORMAL, 0, 1 );
      FT_BitmapGlyph bitmap_glyph = (FT_BitmapGlyph)glyph;
	  FT_Bitmap& bitmap=bitmap_glyph->bitmap;

    BmFont::BmChar Char;
    Char.ch = *it;
    Char.Resize(bitmap.width, bitmap.rows);
    for (int y = 0; y < bitmap.rows; ++y)
      memcpy(&Char.Pix.at(0, y), bitmap.buffer + y*bitmap.width, bitmap.width);
    Char.advx = glyph->advance.x >> 16;
    Char.offx = bitmap_glyph->left;
    Char.offy = -bitmap_glyph->top;
    Font.Chars.push_back(Char);
  }
	FT_Done_Face(face);
	FT_Done_FreeType(library);
}

// -------------------------------------------------------------------

// ---------------------------------
static bool CharGreater(BmFont::BmChar *a, BmFont::BmChar *b)
{
  return std::max(a->w, a->h) > std::max(b->w, b->h);
}

// ---------------------------------
void GenerateFont(BmFont &Font, const char *font, int dpi, int size, int boldf, int italicf, int texsize, const std::vector<wchar_t> &charset)
{
  ProcessTTF(Font, font, dpi, size, boldf, italicf, charset);

  // The RectPlacement array where chars are placed. Each RectPlacement will correspond to a texture
  std::vector<CRectPlacement> Tx;
  Tx.reserve(16); // More than enough to avoid extra copies

  // Sort chars in descending order according to largest dimension
  // This makes insertion in the RectPlacement nearly optimal
  std::vector<BmFont::BmChar *> SortedChars;
  SortedChars.reserve(Font.Chars.size());
  for (std::vector<BmFont::BmChar>::iterator it = Font.Chars.begin(); it != Font.Chars.end(); ++it)
    SortedChars.push_back(&*it);
  std::sort(SortedChars.begin(), SortedChars.end(), CharGreater);

  // Insert chars in the array of rect placements
  for (std::vector<BmFont::BmChar *>::iterator it = SortedChars.begin(); it != SortedChars.end(); ++it)
  {
    // Find if bitmap is identical to already inserted one
    bool bFound = false;
    if ((*it)->w && (*it)->h)
    {
      for (std::vector<BmFont::BmChar *>::iterator previt = SortedChars.begin(); !bFound && previt != it; ++previt)
      {
        if ((*previt)->w == (*it)->w && (*previt)->h == (*it)->h &&
            0 == memcmp(&(*previt)->Pix.pix[0], &(*it)->Pix.pix[0], (*it)->w*(*it)->h))
        {
          bFound = true;
          (*it)->tx = (*previt)->tx;
          (*it)->txu = (*previt)->txu;
          (*it)->txv = (*previt)->txv;
        }
      }
    }

    // Insert the char anew
    if (!bFound)
    {
      CRectPlacement::TRect r(0, 0, (*it)->w + 1, (*it)->h + 1); // Extra room
      CRectPlacement *pTx = 0;
      for (std::vector<CRectPlacement>::iterator pi = Tx.begin(); !pTx && pi != Tx.end(); ++pi)
        if (pi->AddAtEmptySpotAutoGrow(&r, texsize, texsize))
          pTx = &*pi;

      // No space in existing rects, add a new one
      if (!pTx)
      {
        Tx.push_back(CRectPlacement(1, 1));
        if (Tx.back().AddAtEmptySpotAutoGrow(&r, texsize, texsize))
          pTx = &Tx.back();
      }

      // Found an available spot?
      if (pTx)
      {
        (*it)->tx = unsigned(pTx - &Tx.front());
        (*it)->txu = r.x;
        (*it)->txv = r.y;
      }
      else
        throw std::runtime_error("Unable to allocate space in textures");
    }
  }

  // Build the array of textures
  Font.Textures.resize(Tx.size());
  for (unsigned i = 0; i < Tx.size(); ++i)
    Font.Textures[i].Resize(Tx[i].GetW(), Tx[i].GetH());

  // Place char bitmaps in their corresponding textures
  for (std::vector<BmFont::BmChar>::const_iterator it = Font.Chars.begin(); it != Font.Chars.end(); ++it)
  {
    unsigned char *p = &Font.Textures[it->tx].at(it->txu, it->txv);
    unsigned w = Font.Textures[it->tx].w;
    for (unsigned y = 0; y < it->h; ++y)
      memcpy(p + y*w, &it->Pix.at(0, y), it->w);
  }
}
