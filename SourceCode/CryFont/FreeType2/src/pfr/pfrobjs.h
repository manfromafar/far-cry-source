/***************************************************************************/
/*                                                                         */
/*  pfrobjs.h                                                              */
/*                                                                         */
/*    FreeType PFR object methods (specification).                         */
/*                                                                         */
/*  Copyright 2002, 2003 by                                                */
/*  David Turner, Robert Wilhelm, and Werner Lemberg.                      */
/*                                                                         */
/*  This file is part of the FreeType project, and may only be used,       */
/*  modified, and distributed under the terms of the FreeType project      */
/*  license, LICENSE.TXT.  By continuing to use, modify, or distribute     */
/*  this file you indicate that you have read the license and              */
/*  understand and accept it fully.                                        */
/*                                                                         */
/***************************************************************************/


#ifndef __PFROBJS_H__
#define __PFROBJS_H__

#include "pfrtypes.h"


FT_BEGIN_HEADER

  typedef struct PFR_FaceRec_*  PFR_Face;

  typedef struct PFR_SizeRec_*  PFR_Size;

  typedef struct PFR_SlotRec_*  PFR_Slot;


  typedef struct  PFR_FaceRec_
  {
    FT_FaceRec      root;
    PFR_HeaderRec   header;
    PFR_LogFontRec  log_font;
    PFR_PhyFontRec  phy_font;

  } PFR_FaceRec;


  typedef struct  PFR_SizeRec_
  {
    FT_SizeRec  root;

  } PFR_SizeRec;


  typedef struct  PFR_SlotRec_
  {
    FT_GlyphSlotRec  root;
    PFR_GlyphRec     glyph;

  } PFR_SlotRec;


  FT_LOCAL( FT_Error )
  pfr_face_init( FT_Stream      stream,
                 PFR_Face       face,
                 FT_Int         face_index,
                 FT_Int         num_params,
                 FT_Parameter*  params );

  FT_LOCAL( void )
  pfr_face_done( PFR_Face  face );


  FT_LOCAL( void )
  pfr_face_get_kerning( PFR_Face    face,
                        FT_UInt     glyph1,
                        FT_UInt     glyph2,
                        FT_Vector*  kerning );


  FT_LOCAL( FT_Error )
  pfr_slot_init( PFR_Slot  slot );

  FT_LOCAL( void )
  pfr_slot_done( PFR_Slot  slot );


  FT_LOCAL( FT_Error )
  pfr_slot_load( PFR_Slot  slot,
                 PFR_Size  size,
                 FT_UInt   gindex,
                 FT_Int32  load_flags );


FT_END_HEADER

#endif /* __PFROBJS_H__ */


/* END */
