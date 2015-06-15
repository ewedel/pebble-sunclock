/**
 *  @file
 *  
 */


#include  "TransRotBmp.h"

#include  "helpers.h"


TransRotBmp* transrotbmp_create_with_resources(uint32_t residWhiteMask
#ifdef PBL_PLATFORM_APLITE
                                              , uint32_t residBlackMask
#endif
                                               )
{

TransRotBmp* pMyRet;

   pMyRet = (TransRotBmp*) malloc(sizeof(TransRotBmp));
   if (pMyRet == 0)
   {
      return 0;
   }

   pMyRet->pBmpWhiteMask = gbitmap_create_with_resource(residWhiteMask);
#ifdef PBL_PLATFORM_APLITE
   pMyRet->pBmpBlackMask = gbitmap_create_with_resource(residBlackMask);
#endif

   if ((pMyRet->pBmpWhiteMask == 0)
#ifdef PBL_PLATFORM_APLITE
       || (pMyRet->pBmpBlackMask == 0)
#endif
       )
   {
      //  incomplete init, so return null to show this.
      transrotbmp_destroy(pMyRet);
      return 0;
   }

   pMyRet->pRbmpWhiteLayer = rot_bitmap_layer_create(pMyRet->pBmpWhiteMask);
#ifdef PBL_PLATFORM_APLITE
   pMyRet->pRbmpBlackLayer = rot_bitmap_layer_create(pMyRet->pBmpBlackMask);
#endif

   if ((pMyRet->pRbmpWhiteLayer == 0)
#ifdef PBL_PLATFORM_APLITE
       || (pMyRet->pRbmpBlackLayer == 0)
#endif
       )
   {
      //  incomplete init, so return null to show this.
      transrotbmp_destroy(pMyRet);
      return 0;
   }

#ifdef PBL_PLATFORM_APLITE
   //  for aplite use standard "png-trans" mask compositing modes, per this post:
   //    http://forums.getpebble.com/discussion/comment/36006/#Comment_36006
   rot_bitmap_set_compositing_mode(pMyRet->pRbmpWhiteLayer, GCompOpOr);
   rot_bitmap_set_compositing_mode(pMyRet->pRbmpBlackLayer, GCompOpClear);
#elif PBL_PLATFORM_BASALT
   //  for basalt we are supposed to use "png" with GCompOpSet, per
   //    http://developer.getpebble.com/blog/2015/05/13/tips-and-tricks-transparent-images/
   rot_bitmap_set_compositing_mode(pMyRet->pRbmpWhiteLayer, GCompOpSet);
#endif

   return pMyRet;

}  /* end of transrotbmp_create_with_resources */


void  transrotbmp_set_src_ic(TransRotBmp *pTransBmp, GPoint ic)
{
   rot_bitmap_set_src_ic(pTransBmp->pRbmpWhiteLayer, ic);
#ifdef PBL_PLATFORM_APLITE
   rot_bitmap_set_src_ic(pTransBmp->pRbmpBlackLayer, ic);
#endif
}


void  transrotbmp_add_to_layer(TransRotBmp *pTransBmp, Layer *parent)
{
   layer_add_child(parent, (Layer *) pTransBmp->pRbmpWhiteLayer);
#ifdef PBL_PLATFORM_APLITE
   layer_add_child(parent, (Layer *) pTransBmp->pRbmpBlackLayer);
#endif
}


void  transrotbmp_set_angle(TransRotBmp *pTransBmp, int32_t angle)
{
   rot_bitmap_layer_set_angle(pTransBmp->pRbmpWhiteLayer, angle);
#ifdef PBL_PLATFORM_APLITE
   rot_bitmap_layer_set_angle(pTransBmp->pRbmpBlackLayer, angle);
#endif
}

void transrotbmp_set_pos_centered(TransRotBmp *pTransBmp, int32_t offsetX, int32_t offsetY)
{

   //  Logging code shows, in PebbleOS version 2 beta 4, that the black
   //  and white mask layers have the same frame parameters.  Whew!

   GRect frame = layer_get_frame((Layer *) (pTransBmp->pRbmpWhiteLayer));

   frame.origin.x = (144 / 2) + offsetX - (frame.size.w / 2);
   frame.origin.y = (168 / 2) + offsetY - (frame.size.h / 2);

   layer_set_frame((Layer *) (pTransBmp->pRbmpWhiteLayer), frame);

#ifdef PBL_PLATFORM_APLITE
   layer_set_frame((Layer *) (pTransBmp->pRbmpBlackLayer), frame);
#endif

}  /* end of transrotbmp_set_pos_centered */


void  transrotbmp_destroy(TransRotBmp *pTransBmp)
{

   if (pTransBmp == 0)
      return;

   layer_remove_from_parent((Layer *) pTransBmp->pRbmpWhiteLayer);
#ifdef PBL_PLATFORM_APLITE
   layer_remove_from_parent((Layer *) pTransBmp->pRbmpBlackLayer);
#endif

   SAFE_DESTROY(rot_bitmap_layer, pTransBmp->pRbmpWhiteLayer);
#ifdef PBL_PLATFORM_APLITE
   SAFE_DESTROY(rot_bitmap_layer, pTransBmp->pRbmpBlackLayer);
#endif

   SAFE_DESTROY(gbitmap, pTransBmp->pBmpWhiteMask);
#ifdef PBL_PLATFORM_APLITE
   SAFE_DESTROY(gbitmap, pTransBmp->pBmpBlackMask);
#endif

   free(pTransBmp);

   return;

}  /* end of transrotbmp_destroy */


void  transrotbmp_draw_in_rect(TransRotBmp *pTransBmp, GContext* ctx, GRect rect)
{


#ifdef PBL_PLATFORM_APLITE
   //  Per this post by RenaudCazoulat
   //    http://forums.getpebble.com/discussion/comment/36006/#Comment_36006
   //  we want to composite our white mask using GCompOr
   //  and our black mask using GCompClear.

   graphics_context_set_compositing_mode(ctx, GCompOpOr);
   graphics_draw_bitmap_in_rect(ctx, pTransBmp->pBmpWhiteMask, rect);

   graphics_context_set_compositing_mode(ctx, GCompOpClear);
   graphics_draw_bitmap_in_rect(ctx, pTransBmp->pBmpBlackMask, rect);

#elif PBL_PLATFORM_BASALT
   //  for basalt we are supposed to use "png" with GCompOpSet, per
   //    http://developer.getpebble.com/blog/2015/05/13/tips-and-tricks-transparent-images/
   graphics_context_set_compositing_mode(ctx, GCompOpSet);
   graphics_draw_bitmap_in_rect(ctx, pTransBmp->pBmpWhiteMask, rect);
#endif

   return;

}  /* end of transrotbmp_draw_in_rect */




