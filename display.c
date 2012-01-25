#include <proto/graphics.h>
#include <inline/graphics_protos.h>

#include "common.h"
#include "display.h"

struct DBufRaster *NewDBufRaster(SHORT width, SHORT height, SHORT depth) {
  struct DBufRaster *raster = NEW_SZ(struct DBufRaster);

  if (raster) {
    struct ViewPort *viewPort = NEW_SZ(struct ViewPort);

    if (viewPort) {
      InitVPort(viewPort);

      viewPort->DWidth = width;
      viewPort->DHeight = height;
      viewPort->RasInfo = NEW_SZ(struct RasInfo);
      viewPort->ColorMap = GetColorMap(1 << depth);

      struct DBufInfo *dbufInfo = AllocDBufInfo(viewPort);

      raster->DBufInfo = dbufInfo;
      raster->ViewPort = viewPort;

      if (dbufInfo && viewPort->RasInfo && viewPort->ColorMap) {
        dbufInfo->dbi_UserData1 = (APTR)AllocBitMap(width, height, depth,
                                                    BMF_DISPLAYABLE|BMF_CLEAR,
                                                    NULL);
        dbufInfo->dbi_UserData2 = (APTR)AllocBitMap(width, height, depth,
                                                    BMF_DISPLAYABLE|BMF_CLEAR,
                                                    NULL); 

        dbufInfo->dbi_SafeMessage.mn_ReplyPort = CreateMsgPort();
        dbufInfo->dbi_DispMessage.mn_ReplyPort = CreateMsgPort();

        viewPort->RasInfo->BitMap = (struct BitMap *)dbufInfo->dbi_UserData1;
        raster->BitMap = (struct BitMap *)raster->DBufInfo->dbi_UserData2;

        if (dbufInfo->dbi_UserData1 && dbufInfo->dbi_UserData2) {
          raster->SafeToSwap = TRUE;
          raster->SafeToWrite = TRUE;
          raster->CurrentBitMap = 1;

          return raster;
        }
      }
    }

    DeleteDBufRaster(raster);
  }

  return NULL;
}

void DeleteDBufRaster(struct DBufRaster *raster) {
  if (raster->DBufInfo) {
    struct DBufInfo *dbufInfo = raster->DBufInfo;

    FreeBitMap(dbufInfo->dbi_UserData1);
    FreeBitMap(dbufInfo->dbi_UserData2);

    DeleteMsgPort(dbufInfo->dbi_SafeMessage.mn_ReplyPort);
    DeleteMsgPort(dbufInfo->dbi_DispMessage.mn_ReplyPort);

    FreeDBufInfo(dbufInfo);
  }

  if (raster->ViewPort) {
    struct ViewPort *viewPort = raster->ViewPort;

    FreeVPortCopLists(viewPort);
    FreeColorMap(viewPort->ColorMap);
    DELETE(viewPort->RasInfo);
    DELETE(viewPort);
  }
}

void WaitForSafeToWrite(struct DBufRaster *raster) {
  if (!raster->SafeToWrite) {
    struct MsgPort *SafeMsgPort = raster->DBufInfo->dbi_SafeMessage.mn_ReplyPort;

    while (!GetMsg(SafeMsgPort))
      Wait(1L << SafeMsgPort->mp_SigBit);

    raster->SafeToWrite = TRUE;
  }
}

void WaitForSafeToSwap(struct DBufRaster *raster) {
  if (!raster->SafeToSwap) {
    struct MsgPort *DispMsgPort = raster->DBufInfo->dbi_DispMessage.mn_ReplyPort;

    while (!GetMsg(DispMsgPort))
      Wait(1L << DispMsgPort->mp_SigBit);

    raster->SafeToSwap = TRUE;
  }
}

void DBufRasterSwap(struct DBufRaster *raster) {
  ChangeVPBitMap(raster->ViewPort, raster->BitMap, raster->DBufInfo);

  raster->BitMap = (struct BitMap *)(raster->CurrentBitMap ?
                                     raster->DBufInfo->dbi_UserData2 :
                                     raster->DBufInfo->dbi_UserData1);
  raster->SafeToSwap = FALSE;
  raster->SafeToWrite = FALSE;
  raster->CurrentBitMap ^= 1;
}

struct View *NewView()
{
  struct View *view;
  
  if ((view = NEW_SZ(struct View)))
    InitView(view);

  return view;
}

void DeleteView(struct View *view) {
  FreeCprList(view->LOFCprList);

  if (view->SHFCprList)
    FreeCprList(view->SHFCprList);

  DELETE(view);
}

static struct Palette {
  UWORD Count;
  UWORD Start;
  ULONG Components[768];
} Palette;

void ViewPortLoadPalette(struct ViewPort *viewPort, UBYTE *components,
                         UWORD start, UWORD count) {
  int i;

  Palette.Count = count;
  Palette.Start = start;

  for (i = 0; i < count * 3; i++)
    Palette.Components[i] = (ULONG)components[i] << 24;

  LoadRGB32(viewPort, (ULONG *)&Palette);
}
