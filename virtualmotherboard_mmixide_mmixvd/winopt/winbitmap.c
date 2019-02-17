
#include <windows.h>
#include "winopt.h"

//
//	BitmapToRegion :	Create a region from the "non-transparent" pixels of a bitmap
//	Author :			Jean-Edouard Lachand-Robert (http://www.geocities.com/Paris/LeftBank/1160/resume.htm), June 1998.
//
//	hBmp :				Source bitmap
//	cTransparentColor :	Color base for the "transparent" pixels (default is black)
//	cTolerance :		Color tolerance for the "transparent" pixels.
//
//	A pixel is assumed to be transparent if the value of each of its 3 components (blue, green and red) is 
//	greater or equal to the corresponding value in cTransparentColor and is lower or equal to the 
//	corresponding value in cTransparentColor + cTolerance.
//
HRGN BitmapToRegion (HBITMAP hBmp)
{
	HRGN hRgn = NULL;

	if (hBmp)
	{
		// Create a memory DC inside which we will scan the bitmap content
		HDC hMemDC = CreateCompatibleDC(NULL);
		if (hMemDC)
		{
			// Get bitmap size
			BITMAP bm;
			BITMAPINFOHEADER RGB32BITSBITMAPINFO;
			VOID * pbits32; 
			HBITMAP hbm32; 
			
			GetObject(hBmp, sizeof(bm), &bm);

			ZeroMemory(&RGB32BITSBITMAPINFO, sizeof(RGB32BITSBITMAPINFO));

			RGB32BITSBITMAPINFO.biSize=sizeof(BITMAPINFOHEADER);
			RGB32BITSBITMAPINFO.biWidth=		bm.bmWidth;
			RGB32BITSBITMAPINFO.biHeight=		bm.bmHeight;
			RGB32BITSBITMAPINFO.biPlanes=		1;
			RGB32BITSBITMAPINFO.biBitCount=		32;

			RGB32BITSBITMAPINFO.biCompression=  BI_RGB;

            hbm32 = CreateDIBSection(hMemDC, (BITMAPINFO *)&RGB32BITSBITMAPINFO, DIB_RGB_COLORS, &pbits32, NULL, 0);
		
			if (hbm32)
			{
				HBITMAP holdBmp = (HBITMAP)SelectObject(hMemDC, hbm32);

				// Create a DC just to copy the bitmap into the memory DC
				HDC hDC = CreateCompatibleDC(hMemDC);
				if (hDC)
				{
					// Get how many bytes per row we have for the bitmap bits (rounded up to 32 bits)
					BITMAP bm32;
					HBITMAP holdBmp;
					DWORD maxRects;
					HANDLE hData; 
					RGNDATA *pData;
					BYTE *p32,lr,lg,lb;
					HRGN h;
					int x,y;
					GetObject(hbm32, sizeof(bm32), &bm32);
					while (bm32.bmWidthBytes % 4)
						bm32.bmWidthBytes++;

					// Copy the bitmap into the memory DC
					holdBmp = (HBITMAP)SelectObject(hDC, hBmp);
					BitBlt(hMemDC, 0, 0, bm.bmWidth, bm.bmHeight, hDC, 0, 0, SRCCOPY);

					// For better performances, we will use the ExtCreateRegion() function to create the
					// region. This function take a RGNDATA structure on entry. We will add rectangles by
					// amount of ALLOC_UNIT number in this structure.
					#define ALLOC_UNIT	100
					maxRects = ALLOC_UNIT;
					hData = GlobalAlloc(GMEM_MOVEABLE, sizeof(RGNDATAHEADER) + (sizeof(RECT) * maxRects));
					pData = (RGNDATA *)GlobalLock(hData);
					pData->rdh.dwSize = sizeof(RGNDATAHEADER);
					pData->rdh.iType = RDH_RECTANGLES;
					pData->rdh.nCount = pData->rdh.nRgnSize = 0;
					SetRect(&pData->rdh.rcBound, MAXLONG, MAXLONG, 0, 0);


					// Scan each bitmap row from bottom to top (the bitmap is inverted vertically)
					p32 = (BYTE *)bm32.bmBits + (bm32.bmHeight - 1) * bm32.bmWidthBytes;

					// Keep on hand highest and lowest values for the "transparent" pixels
				    lr = GetRValue(*(LONG *)p32);
					lg = GetGValue(*(LONG *)p32);
					lb = GetBValue(*(LONG *)p32);

					for (y = 0; y < bm.bmHeight; y++)
					{
						// Scan each bitmap pixel from left to right
						for (x = 0; x < bm.bmWidth; x++)
						{
							// Search for a continuous range of "non transparent pixels"
							int x0 = x;
							LONG *p = (LONG *)p32 + x;
							while (x < bm.bmWidth)
							{
								BYTE b = GetRValue(*p);
								if (b == lr)
								{
									b = GetGValue(*p);
									if (b == lg )
									{
										b = GetBValue(*p);
										if (b == lb )
											// This pixel is "transparent"
											break;
									}
								}
								p++;
								x++;
							}

							if (x > x0)
							{   RECT *pr;
								// Add the pixels (x0, y) to (x, y+1) as a new rectangle in the region
								if (pData->rdh.nCount >= maxRects)
								{
									GlobalUnlock(hData);
									maxRects += ALLOC_UNIT;
									hData = GlobalReAlloc(hData, sizeof(RGNDATAHEADER) + (sizeof(RECT) * maxRects), GMEM_MOVEABLE);
									pData = (RGNDATA *)GlobalLock(hData);
								}
								pr = (RECT *)&pData->Buffer;
								SetRect(&pr[pData->rdh.nCount], x0, y, x, y+1);
								if (x0 < pData->rdh.rcBound.left)
									pData->rdh.rcBound.left = x0;
								if (y < pData->rdh.rcBound.top)
									pData->rdh.rcBound.top = y;
								if (x > pData->rdh.rcBound.right)
									pData->rdh.rcBound.right = x;
								if (y+1 > pData->rdh.rcBound.bottom)
									pData->rdh.rcBound.bottom = y+1;
								pData->rdh.nCount++;

								// On Windows98, ExtCreateRegion() may fail if the number of rectangles is too
								// large (ie: > 4000). Therefore, we have to create the region by multiple steps.
								if (pData->rdh.nCount == 2000)
								{
									HRGN h = ExtCreateRegion(NULL, sizeof(RGNDATAHEADER) + (sizeof(RECT) * maxRects), pData);
									if (hRgn)
									{
										CombineRgn(hRgn, hRgn, h, RGN_OR);
										DeleteObject(h);
									}
									else
										hRgn = h;
									pData->rdh.nCount = 0;
									SetRect(&pData->rdh.rcBound, MAXLONG, MAXLONG, 0, 0);
								}
							}
						}

						// Go to next row (remember, the bitmap is inverted vertically)
						p32 -= bm32.bmWidthBytes;
					}

					// Create or extend the region with the remaining rectangles
					h = ExtCreateRegion(NULL, sizeof(RGNDATAHEADER) + (sizeof(RECT) * maxRects), pData);
					if (hRgn)
					{
						CombineRgn(hRgn, hRgn, h, RGN_OR);
						DeleteObject(h);
					}
					else
						hRgn = h;

					// Clean up
					SelectObject(hDC, holdBmp);
					DeleteDC(hDC);
				}

				DeleteObject(SelectObject(hMemDC, holdBmp));
			}

			DeleteDC(hMemDC);
		}	
	}

	return hRgn;
}
