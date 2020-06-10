/* The copyright in this software is being made available under the BSD
 * License, included below. This software may be subject to other third party
 * and contributor rights, including patent rights, and no such rights are
 * granted under this license.
 *
 * Copyright (c) 2010-2020, ITU/ISO/IEC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
 *    be used to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

/** \file     Slice.cpp
    \brief    slice header and SPS class
*/

#include "CommonDef.h"
#include "Unit.h"
#include "Slice.h"
#include "Picture.h"
#include "dtrace_next.h"

#include "UnitTools.h"

//! \ingroup CommonLib
//! \{

Slice::Slice()
: m_iPOC                          ( 0 )
, m_iLastIDR                      ( 0 )
, m_iAssociatedIRAP               ( 0 )
, m_iAssociatedIRAPType           ( NAL_UNIT_INVALID )
, m_rpl0Idx                       ( -1 )
, m_rpl1Idx                       ( -1 )
#if JVET_Q0155_COLOUR_ID
, m_colourPlaneId                 ( 0 )
#endif
, m_eNalUnitType                  ( NAL_UNIT_CODED_SLICE_IDR_W_RADL )
#if JVET_Q0775_PH_IN_SH
, m_pictureHeaderInSliceHeader   ( false )
#endif
, m_eSliceType                    ( I_SLICE )
, m_iSliceQp                      ( 0 )
, m_ChromaQpAdjEnabled            ( false )
#if JVET_Q0346_LMCS_ENABLE_IN_SH
, m_lmcsEnabledFlag               ( 0 )
#endif
#if JVET_Q0346_SCALING_LIST_USED_IN_SH
, m_explicitScalingListUsed       ( 0 )
#endif
, m_deblockingFilterDisable       ( false )
, m_deblockingFilterOverrideFlag  ( false )
, m_deblockingFilterBetaOffsetDiv2( 0 )
, m_deblockingFilterTcOffsetDiv2  ( 0 )
#if JVET_Q0121_DEBLOCKING_CONTROL_PARAMETERS
, m_deblockingFilterCbBetaOffsetDiv2( 0 )
, m_deblockingFilterCbTcOffsetDiv2  ( 0 )
, m_deblockingFilterCrBetaOffsetDiv2( 0 )
, m_deblockingFilterCrTcOffsetDiv2  ( 0 )
#endif
#if JVET_Q0089_SLICE_LOSSLESS_CODING_CHROMA_BDPCM
, m_tsResidualCodingDisabledFlag  ( false )
#endif
, m_pendingRasInit                ( false )
, m_bCheckLDC                     ( false )
, m_biDirPred                    ( false )
, m_iSliceQpDelta                 ( 0 )
, m_iDepth                        ( 0 )
#if !JVET_Q0117_PARAMETER_SETS_CLEANUP
, m_dps                           ( nullptr )
#endif
, m_pcSPS                         ( NULL )
, m_pcPPS                         ( NULL )
, m_pcPic                         ( NULL )
, m_pcPicHeader                   ( NULL )
, m_colFromL0Flag                 ( true )
#if !SPS_ID_CHECK
, m_noIncorrectPicOutputFlag      ( false )
, m_handleCraAsCvsStartFlag       ( false )
#endif
, m_colRefIdx                     ( 0 )
, m_uiTLayer                      ( 0 )
, m_bTLayerSwitchingFlag          ( false )
, m_independentSliceIdx           ( 0 )
, m_nextSlice                     ( false )
, m_sliceBits                     ( 0 )
, m_bFinalized                    ( false )
, m_bTestWeightPred               ( false )
, m_bTestWeightBiPred             ( false )
, m_substreamSizes                ( )
, m_numEntryPoints                ( 0 )
, m_cabacInitFlag                 ( false )
 , m_sliceSubPicId               ( 0 )
, m_encCABACTableIdx              (I_SLICE)
, m_iProcessingStartTime          ( 0 )
, m_dProcessingTime               ( 0 )
{
  for(uint32_t i=0; i<NUM_REF_PIC_LIST_01; i++)
  {
    m_aiNumRefIdx[i] = 0;
  }

  for (uint32_t component = 0; component < MAX_NUM_COMPONENT; component++)
  {
    m_lambdas            [component] = 0.0;
    m_iSliceChromaQpDelta[component] = 0;
  }
  m_iSliceChromaQpDelta[JOINT_CbCr] = 0;

  initEqualRef();

  for ( int idx = 0; idx < MAX_NUM_REF; idx++ )
  {
    m_list1IdxToList0Idx[idx] = -1;
  }

  for(int iNumCount = 0; iNumCount < MAX_NUM_REF; iNumCount++)
  {
    for(uint32_t i=0; i<NUM_REF_PIC_LIST_01; i++)
    {
      m_apcRefPicList [i][iNumCount] = NULL;
      m_aiRefPOCList  [i][iNumCount] = 0;
    }
  }

  resetWpScaling();
  initWpAcDcParam();

  for(int ch=0; ch < MAX_NUM_CHANNEL_TYPE; ch++)
  {
    m_saoEnabledFlag[ch] = false;
  }

  memset(m_alfApss, 0, sizeof(m_alfApss));
#if JVET_Q0795_CCALF
  m_ccAlfFilterParam.reset();
  resetTileGroupAlfEnabledFlag();
  resetTileGroupCcAlCbfEnabledFlag();
  resetTileGroupCcAlCrfEnabledFlag();
#endif

  m_sliceMap.initSliceMap();
}

Slice::~Slice()
{
  m_sliceMap.initSliceMap();
}


void Slice::initSlice()
{
  for(uint32_t i=0; i<NUM_REF_PIC_LIST_01; i++)
  {
    m_aiNumRefIdx[i]      = 0;
  }
  m_colFromL0Flag = true;
#if JVET_Q0155_COLOUR_ID
  m_colourPlaneId = 0;
#endif
  m_colRefIdx = 0;
#if JVET_Q0346_LMCS_ENABLE_IN_SH
  m_lmcsEnabledFlag = 0;
#endif
#if JVET_Q0346_SCALING_LIST_USED_IN_SH
  m_explicitScalingListUsed = 0;
#endif
  initEqualRef();

  m_bCheckLDC = false;

  m_biDirPred = false;
  m_symRefIdx[0] = -1;
  m_symRefIdx[1] = -1;

  for (uint32_t component = 0; component < MAX_NUM_COMPONENT; component++)
  {
    m_iSliceChromaQpDelta[component] = 0;
  }
  m_iSliceChromaQpDelta[JOINT_CbCr] = 0;


  m_bFinalized=false;

  m_substreamSizes.clear();
  m_cabacInitFlag        = false;
  m_enableDRAPSEI        = false;
  m_useLTforDRAP         = false;
  m_isDRAP               = false;
  m_latestDRAPPOC        = MAX_INT;
  resetTileGroupAlfEnabledFlag();
#if JVET_Q0795_CCALF
  m_ccAlfFilterParam.reset();
  m_tileGroupCcAlfCbEnabledFlag = 0;
  m_tileGroupCcAlfCrEnabledFlag = 0;
  m_tileGroupCcAlfCbApsId = -1;
  m_tileGroupCcAlfCrApsId = -1;
#endif
}

void Slice::inheritFromPicHeader( PicHeader *picHeader, const PPS *pps, const SPS *sps )
{ 
#if JVET_Q0819_PH_CHANGES 
  if (pps->getRplInfoInPhFlag())
#else
  if(picHeader->getPicRplPresentFlag())
#endif
  {
    setRPL0idx( picHeader->getRPL0idx() );
    *getLocalRPL0() = *picHeader->getLocalRPL0();
    if(getRPL0idx() != -1)
    {
      setRPL0(sps->getRPLList0()->getReferencePictureList(getRPL0idx()));
    }
    else
    {
      setRPL0(getLocalRPL0());
    }
    
    setRPL1idx( picHeader->getRPL1idx() );
    *getLocalRPL1() = *picHeader->getLocalRPL1();
    if(getRPL1idx() != -1)
    {
      setRPL1(sps->getRPLList1()->getReferencePictureList(getRPL1idx()));
    }
    else
    {
      setRPL1(getLocalRPL1());
    }
  }

  setDeblockingFilterDisable( picHeader->getDeblockingFilterDisable() );
  setDeblockingFilterBetaOffsetDiv2( picHeader->getDeblockingFilterBetaOffsetDiv2() );
  setDeblockingFilterTcOffsetDiv2( picHeader->getDeblockingFilterTcOffsetDiv2() );
#if JVET_Q0121_DEBLOCKING_CONTROL_PARAMETERS
  setDeblockingFilterCbBetaOffsetDiv2( picHeader->getDeblockingFilterCbBetaOffsetDiv2() );
  setDeblockingFilterCbTcOffsetDiv2( picHeader->getDeblockingFilterCbTcOffsetDiv2() );
  setDeblockingFilterCrBetaOffsetDiv2( picHeader->getDeblockingFilterCrBetaOffsetDiv2() );
  setDeblockingFilterCrTcOffsetDiv2( picHeader->getDeblockingFilterCrTcOffsetDiv2() );
#endif

  setSaoEnabledFlag(CHANNEL_TYPE_LUMA,     picHeader->getSaoEnabledFlag(CHANNEL_TYPE_LUMA));
  setSaoEnabledFlag(CHANNEL_TYPE_CHROMA,   picHeader->getSaoEnabledFlag(CHANNEL_TYPE_CHROMA));

  setTileGroupAlfEnabledFlag(COMPONENT_Y,  picHeader->getAlfEnabledFlag(COMPONENT_Y));
  setTileGroupAlfEnabledFlag(COMPONENT_Cb, picHeader->getAlfEnabledFlag(COMPONENT_Cb));
  setTileGroupAlfEnabledFlag(COMPONENT_Cr, picHeader->getAlfEnabledFlag(COMPONENT_Cr));
  setTileGroupNumAps(picHeader->getNumAlfAps());
  setAlfAPSs(picHeader->getAlfAPSs());
  setTileGroupApsIdChroma(picHeader->getAlfApsIdChroma());
#if JVET_Q0795_CCALF
  setTileGroupCcAlfCbEnabledFlag(picHeader->getCcAlfEnabledFlag(COMPONENT_Cb));
  setTileGroupCcAlfCrEnabledFlag(picHeader->getCcAlfEnabledFlag(COMPONENT_Cr));
  setTileGroupCcAlfCbApsId(picHeader->getCcAlfCbApsId());
  setTileGroupCcAlfCrApsId(picHeader->getCcAlfCrApsId());
  m_ccAlfFilterParam.ccAlfFilterEnabled[COMPONENT_Cb - 1] = picHeader->getCcAlfEnabledFlag(COMPONENT_Cb);
  m_ccAlfFilterParam.ccAlfFilterEnabled[COMPONENT_Cr - 1] = picHeader->getCcAlfEnabledFlag(COMPONENT_Cr);
#endif
}

#if JVET_Q0151_Q0205_ENTRYPOINTS
void Slice::setNumSubstream(const SPS* sps, const PPS* pps) 
{
  uint32_t ctuAddr, ctuX, ctuY;
  m_numSubstream = 0;

  // count the number of CTUs that align with either the start of a tile, or with an entropy coding sync point
  // ignore the first CTU since it doesn't count as an entry point
  for (uint32_t i = 1; i < m_sliceMap.getNumCtuInSlice(); i++)
  {
    ctuAddr = m_sliceMap.getCtuAddrInSlice(i);
    ctuX    = (ctuAddr % pps->getPicWidthInCtu());
    ctuY    = (ctuAddr / pps->getPicWidthInCtu());

    if (pps->ctuIsTileColBd(ctuX) && (pps->ctuIsTileRowBd(ctuY) || sps->getEntropyCodingSyncEnabledFlag()))
    {
      m_numSubstream++;
    }
  }
}

void Slice::setNumEntryPoints(const SPS *sps, const PPS *pps)
#else
void  Slice::setNumEntryPoints( const PPS *pps ) 
#endif
{
  uint32_t ctuAddr, ctuX, ctuY;
#if JVET_Q0151_Q0205_ENTRYPOINTS
  uint32_t prevCtuAddr, prevCtuX, prevCtuY;
#endif
  m_numEntryPoints = 0;

  // count the number of CTUs that align with either the start of a tile, or with an entropy coding sync point
  // ignore the first CTU since it doesn't count as an entry point
  for( uint32_t i = 1; i < m_sliceMap.getNumCtuInSlice(); i++ ) 
  {
    ctuAddr = m_sliceMap.getCtuAddrInSlice( i );
    ctuX = ( ctuAddr % pps->getPicWidthInCtu() );
    ctuY = ( ctuAddr / pps->getPicWidthInCtu() );
#if JVET_Q0151_Q0205_ENTRYPOINTS
    prevCtuAddr = m_sliceMap.getCtuAddrInSlice(i - 1);
    prevCtuX    = (prevCtuAddr % pps->getPicWidthInCtu());
    prevCtuY    = (prevCtuAddr / pps->getPicWidthInCtu());

    if (pps->ctuToTileRowBd(ctuY) != pps->ctuToTileRowBd(prevCtuY) || pps->ctuToTileColBd(ctuX) != pps->ctuToTileColBd(prevCtuX) || (ctuY != prevCtuY && sps->getEntropyCodingSyncEntryPointsPresentFlag()))
#else
    if( pps->ctuIsTileColBd( ctuX ) && (pps->ctuIsTileRowBd( ctuY ) || pps->getEntropyCodingSyncEnabledFlag() ) ) 
#endif
    {
      m_numEntryPoints++;
    }
  }
}

void Slice::setDefaultClpRng( const SPS& sps )
{
  m_clpRngs.comp[COMPONENT_Y].min = m_clpRngs.comp[COMPONENT_Cb].min  = m_clpRngs.comp[COMPONENT_Cr].min = 0;
  m_clpRngs.comp[COMPONENT_Y].max                                     = (1<< sps.getBitDepth(CHANNEL_TYPE_LUMA))-1;
  m_clpRngs.comp[COMPONENT_Y].bd  = sps.getBitDepth(CHANNEL_TYPE_LUMA);
  m_clpRngs.comp[COMPONENT_Y].n   = 0;
  m_clpRngs.comp[COMPONENT_Cb].max = m_clpRngs.comp[COMPONENT_Cr].max = (1<< sps.getBitDepth(CHANNEL_TYPE_CHROMA))-1;
  m_clpRngs.comp[COMPONENT_Cb].bd  = m_clpRngs.comp[COMPONENT_Cr].bd  = sps.getBitDepth(CHANNEL_TYPE_CHROMA);
  m_clpRngs.comp[COMPONENT_Cb].n   = m_clpRngs.comp[COMPONENT_Cr].n   = 0;
  m_clpRngs.used = m_clpRngs.chroma = false;
}


bool Slice::getRapPicFlag() const
{
  return getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_W_RADL
      || getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_N_LP
    || getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA;
}


void  Slice::sortPicList        (PicList& rcListPic)
{
  Picture*    pcPicExtract;
  Picture*    pcPicInsert;

  PicList::iterator    iterPicExtract;
  PicList::iterator    iterPicExtract_1;
  PicList::iterator    iterPicInsert;

  for (int i = 1; i < (int)(rcListPic.size()); i++)
  {
    iterPicExtract = rcListPic.begin();
    for (int j = 0; j < i; j++)
    {
      iterPicExtract++;
    }
    pcPicExtract = *(iterPicExtract);

    iterPicInsert = rcListPic.begin();
    while (iterPicInsert != iterPicExtract)
    {
      pcPicInsert = *(iterPicInsert);
      if (pcPicInsert->getPOC() >= pcPicExtract->getPOC())
      {
        break;
      }

      iterPicInsert++;
    }

    iterPicExtract_1 = iterPicExtract;    iterPicExtract_1++;

    //  swap iterPicExtract and iterPicInsert, iterPicExtract = curr. / iterPicInsert = insertion position
    rcListPic.insert( iterPicInsert, iterPicExtract, iterPicExtract_1 );
    rcListPic.erase( iterPicExtract );
  }
}

Picture* Slice::xGetRefPic( PicList& rcListPic, int poc, const int layerId )
{
  PicList::iterator  iterPic = rcListPic.begin();
  Picture*           pcPic   = *(iterPic);

  while ( iterPic != rcListPic.end() )
  {
    if( pcPic->getPOC() == poc && pcPic->layerId == layerId )
    {
      break;
    }
    iterPic++;
    pcPic = *(iterPic);
  }
  return  pcPic;
}

Picture* Slice::xGetLongTermRefPic( PicList& rcListPic, int poc, bool pocHasMsb, const int layerId )
{
  PicList::iterator  iterPic = rcListPic.begin();
  Picture*           pcPic   = *(iterPic);
  Picture*           pcStPic = pcPic;

  int pocCycle = 1 << getSPS()->getBitsForPOC();
  if (!pocHasMsb)
  {
    poc = poc & (pocCycle - 1);
  }

  while ( iterPic != rcListPic.end() )
  {
    pcPic = *(iterPic);
    if( pcPic && pcPic->getPOC() != this->getPOC() && pcPic->referenced && pcPic->layerId == layerId )
    {
      int picPoc = pcPic->getPOC();
      if (!pocHasMsb)
      {
        picPoc = picPoc & (pocCycle - 1);
      }

      if (poc == picPoc)
      {
        if(pcPic->longTerm)
        {
          return pcPic;
        }
        else
        {
          pcStPic = pcPic;
        }
        break;
      }
    }

    iterPic++;
  }

  return pcStPic;
}

void Slice::setRefPOCList       ()
{
  for (int iDir = 0; iDir < NUM_REF_PIC_LIST_01; iDir++)
  {
    for (int iNumRefIdx = 0; iNumRefIdx < m_aiNumRefIdx[iDir]; iNumRefIdx++)
    {
      m_aiRefPOCList[iDir][iNumRefIdx] = m_apcRefPicList[iDir][iNumRefIdx]->getPOC();
    }
  }

}

void Slice::setList1IdxToList0Idx()
{
  int idxL0, idxL1;
  for ( idxL1 = 0; idxL1 < getNumRefIdx( REF_PIC_LIST_1 ); idxL1++ )
  {
    m_list1IdxToList0Idx[idxL1] = -1;
    for ( idxL0 = 0; idxL0 < getNumRefIdx( REF_PIC_LIST_0 ); idxL0++ )
    {
      if ( m_apcRefPicList[REF_PIC_LIST_0][idxL0]->getPOC() == m_apcRefPicList[REF_PIC_LIST_1][idxL1]->getPOC() )
      {
        m_list1IdxToList0Idx[idxL1] = idxL0;
        break;
      }
    }
  }
}

void Slice::constructRefPicList(PicList& rcListPic)
{
  ::memset(m_bIsUsedAsLongTerm, 0, sizeof(m_bIsUsedAsLongTerm));
  if (m_eSliceType == I_SLICE)
  {
    ::memset(m_apcRefPicList, 0, sizeof(m_apcRefPicList));
    ::memset(m_aiNumRefIdx, 0, sizeof(m_aiNumRefIdx));
    return;
  }

  Picture*  pcRefPic = NULL;
  uint32_t numOfActiveRef = 0;
  //construct L0
  numOfActiveRef = getNumRefIdx(REF_PIC_LIST_0);
  int layerIdx = m_pcPic->cs->vps == nullptr ? 0 : m_pcPic->cs->vps->getGeneralLayerIdx( m_pcPic->layerId );

  for (int ii = 0; ii < numOfActiveRef; ii++)
  {
    if( m_pRPL0->isInterLayerRefPic( ii ) )
    {
      CHECK( m_pRPL0->getInterLayerRefPicIdx( ii ) == NOT_VALID, "Wrong ILRP index" );

      int refLayerId = m_pcPic->cs->vps->getLayerId( m_pcPic->cs->vps->getDirectRefLayerIdx( layerIdx, m_pRPL0->getInterLayerRefPicIdx( ii ) ) );

      pcRefPic = xGetRefPic( rcListPic, getPOC(), refLayerId );
      pcRefPic->longTerm = true;
    }
    else
    if (!m_pRPL0->isRefPicLongterm(ii))
    {
      pcRefPic = xGetRefPic( rcListPic, getPOC() - m_pRPL0->getRefPicIdentifier( ii ), m_pcPic->layerId );
      pcRefPic->longTerm = false;
    }
    else
    {
      int pocBits = getSPS()->getBitsForPOC();
      int pocMask = (1 << pocBits) - 1;
      int ltrpPoc = m_pRPL0->getRefPicIdentifier(ii) & pocMask;
      if(m_localRPL0.getDeltaPocMSBPresentFlag(ii))
      {
        ltrpPoc += getPOC() - m_localRPL0.getDeltaPocMSBCycleLT(ii) * (pocMask + 1) - (getPOC() & pocMask);
      }
      pcRefPic = xGetLongTermRefPic( rcListPic, ltrpPoc, m_localRPL0.getDeltaPocMSBPresentFlag( ii ), m_pcPic->layerId );
      pcRefPic->longTerm = true;
    }
    pcRefPic->extendPicBorder();
    m_apcRefPicList[REF_PIC_LIST_0][ii] = pcRefPic;
    m_bIsUsedAsLongTerm[REF_PIC_LIST_0][ii] = pcRefPic->longTerm;
  }

  //construct L1
  numOfActiveRef = getNumRefIdx(REF_PIC_LIST_1);
  for (int ii = 0; ii < numOfActiveRef; ii++)
  {
    if( m_pRPL1->isInterLayerRefPic( ii ) )
    {
      CHECK( m_pRPL1->getInterLayerRefPicIdx( ii ) == NOT_VALID, "Wrong ILRP index" );

      int refLayerId = m_pcPic->cs->vps->getLayerId( m_pcPic->cs->vps->getDirectRefLayerIdx( layerIdx, m_pRPL1->getInterLayerRefPicIdx( ii ) ) );

      pcRefPic = xGetRefPic( rcListPic, getPOC(), refLayerId );
      pcRefPic->longTerm = true;
    }
    else
    if (!m_pRPL1->isRefPicLongterm(ii))
    {
      pcRefPic = xGetRefPic( rcListPic, getPOC() - m_pRPL1->getRefPicIdentifier( ii ), m_pcPic->layerId );
      pcRefPic->longTerm = false;
    }
    else
    {
      int pocBits = getSPS()->getBitsForPOC();
      int pocMask = (1 << pocBits) - 1;
      int ltrpPoc = m_pRPL1->getRefPicIdentifier(ii) & pocMask;
      if(m_localRPL1.getDeltaPocMSBPresentFlag(ii))
      {
        ltrpPoc += getPOC() - m_localRPL1.getDeltaPocMSBCycleLT(ii) * (pocMask + 1) - (getPOC() & pocMask);
      }
      pcRefPic = xGetLongTermRefPic( rcListPic, ltrpPoc, m_localRPL1.getDeltaPocMSBPresentFlag( ii ), m_pcPic->layerId );
      pcRefPic->longTerm = true;
    }
    pcRefPic->extendPicBorder();
    m_apcRefPicList[REF_PIC_LIST_1][ii] = pcRefPic;
    m_bIsUsedAsLongTerm[REF_PIC_LIST_1][ii] = pcRefPic->longTerm;
  }
}

void Slice::initEqualRef()
{
  for (int iDir = 0; iDir < NUM_REF_PIC_LIST_01; iDir++)
  {
    for (int iRefIdx1 = 0; iRefIdx1 < MAX_NUM_REF; iRefIdx1++)
    {
      for (int iRefIdx2 = iRefIdx1; iRefIdx2 < MAX_NUM_REF; iRefIdx2++)
      {
        m_abEqualRef[iDir][iRefIdx1][iRefIdx2] = m_abEqualRef[iDir][iRefIdx2][iRefIdx1] = (iRefIdx1 == iRefIdx2? true : false);
      }
    }
  }
}

void Slice::checkColRefIdx(uint32_t curSliceSegmentIdx, const Picture* pic)
{
  int i;
  Slice* curSlice = pic->slices[curSliceSegmentIdx];
  int currColRefPOC =  curSlice->getRefPOC( RefPicList(1 - curSlice->getColFromL0Flag()), curSlice->getColRefIdx());

  for(i=curSliceSegmentIdx-1; i>=0; i--)
  {
    const Slice* preSlice = pic->slices[i];
    if(preSlice->getSliceType() != I_SLICE)
    {
      const int preColRefPOC  = preSlice->getRefPOC( RefPicList(1 - preSlice->getColFromL0Flag()), preSlice->getColRefIdx());
      if(currColRefPOC != preColRefPOC)
      {
        THROW("Collocated_ref_idx shall always be the same for all slices of a coded picture!");
      }
      else
      {
        break;
      }
    }
  }
}

#if JVET_P0978_RPL_RESTRICTIONS
void Slice::checkCRA(const ReferencePictureList* pRPL0, const ReferencePictureList* pRPL1, const int pocCRA, PicList& rcListPic)
#else
void Slice::checkCRA(const ReferencePictureList *pRPL0, const ReferencePictureList *pRPL1, int& pocCRA, NalUnitType& associatedIRAPType, PicList& rcListPic)
#endif
{
  if (pocCRA < MAX_UINT && getPOC() > pocCRA)
  {
    uint32_t numRefPic = pRPL0->getNumberOfShorttermPictures() + pRPL0->getNumberOfLongtermPictures();
    for (int i = 0; i < numRefPic; i++)
    {
      if (!pRPL0->isRefPicLongterm(i))
      {
        CHECK(getPOC() - pRPL0->getRefPicIdentifier(i) < pocCRA, "Invalid state");
      }
      else
      {
        int pocBits = getSPS()->getBitsForPOC();
        int pocMask = (1 << pocBits) - 1;
        int ltrpPoc = pRPL0->getRefPicIdentifier(i) & pocMask;
        if(pRPL0->getDeltaPocMSBPresentFlag(i))
        {
          ltrpPoc += getPOC() - pRPL0->getDeltaPocMSBCycleLT(i) * (pocMask + 1) - (getPOC() & pocMask);
        }
        CHECK( xGetLongTermRefPic( rcListPic, ltrpPoc, pRPL0->getDeltaPocMSBPresentFlag( i ), m_pcPic->layerId )->getPOC() < pocCRA, "Invalid state" );
      }
    }
    numRefPic = pRPL1->getNumberOfShorttermPictures() + pRPL1->getNumberOfLongtermPictures();
    for (int i = 0; i < numRefPic; i++)
    {
      if (!pRPL1->isRefPicLongterm(i))
      {
        CHECK(getPOC() - pRPL1->getRefPicIdentifier(i) < pocCRA, "Invalid state");
      }
      else if( !pRPL1->isInterLayerRefPic( i ) )
      {
        int pocBits = getSPS()->getBitsForPOC();
        int pocMask = (1 << pocBits) - 1;
        int ltrpPoc = m_pRPL1->getRefPicIdentifier(i) & pocMask;
        if(pRPL1->getDeltaPocMSBPresentFlag(i))
        {
          ltrpPoc += getPOC() - pRPL1->getDeltaPocMSBCycleLT(i) * (pocMask + 1) - (getPOC() & pocMask);
        }
        CHECK( xGetLongTermRefPic( rcListPic, ltrpPoc, pRPL1->getDeltaPocMSBPresentFlag( i ), m_pcPic->layerId )->getPOC() < pocCRA, "Invalid state" );
      }
    }
  }
#if !JVET_P0978_RPL_RESTRICTIONS
  if (getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_W_RADL || getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_N_LP) // IDR picture found
  {
    pocCRA = getPOC();
    associatedIRAPType = getNalUnitType();
  }
  else if (getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA) // CRA picture found
  {
    pocCRA = getPOC();
    associatedIRAPType = getNalUnitType();
  }
#endif
}

#if JVET_P0978_RPL_RESTRICTIONS
void Slice::checkRPL(const ReferencePictureList* pRPL0, const ReferencePictureList* pRPL1, const int associatedIRAPDecodingOrderNumber, PicList& rcListPic)
{
  Picture* pcRefPic;
  int refPicPOC;
  int refPicDecodingOrderNumber;

  int irapPOC = getAssociatedIRAPPOC();

  int numEntriesL0 = pRPL0->getNumberOfShorttermPictures() + pRPL0->getNumberOfLongtermPictures() + pRPL0->getNumberOfInterLayerPictures();
  int numEntriesL1 = pRPL1->getNumberOfShorttermPictures() + pRPL1->getNumberOfLongtermPictures() + pRPL1->getNumberOfInterLayerPictures();

  int numActiveEntriesL0 = getNumRefIdx(REF_PIC_LIST_0);
  int numActiveEntriesL1 = getNumRefIdx(REF_PIC_LIST_1);

#if JVET_Q0042_VUI
  bool fieldSeqFlag = getSPS()->getFieldSeqFlag();
#else
  bool fieldSeqFlag = getSPS()->getVuiParameters() && getSPS()->getVuiParameters()->getFieldSeqFlag();
#endif

  int currentPictureIsTrailing = 0;
  if (getPic()->getDecodingOrderNumber() > associatedIRAPDecodingOrderNumber)
  {
    switch (m_eNalUnitType)
    {
    case NAL_UNIT_CODED_SLICE_STSA:
    case NAL_UNIT_CODED_SLICE_IDR_W_RADL:
    case NAL_UNIT_CODED_SLICE_IDR_N_LP:
    case NAL_UNIT_CODED_SLICE_CRA:
    case NAL_UNIT_CODED_SLICE_RADL:
    case NAL_UNIT_CODED_SLICE_RASL:
      currentPictureIsTrailing = 0;
      break;
    default:
      currentPictureIsTrailing = 1;
    }
  }

  for (int i = 0; i < numEntriesL0; i++)
  {
    if (!pRPL0->isRefPicLongterm(i))
    {
      refPicPOC = getPOC() - pRPL0->getRefPicIdentifier(i);
      pcRefPic = xGetRefPic(rcListPic, refPicPOC, m_pcPic->layerId);
    }
    else
    {
      int pocBits = getSPS()->getBitsForPOC();
      int pocMask = (1 << pocBits) - 1;
      int ltrpPoc = pRPL0->getRefPicIdentifier(i) & pocMask;
      if(pRPL0->getDeltaPocMSBPresentFlag(i))
      {
        ltrpPoc += getPOC() - pRPL0->getDeltaPocMSBCycleLT(i) * (pocMask + 1) - (getPOC() & pocMask);
      }
      pcRefPic = xGetLongTermRefPic(rcListPic, ltrpPoc, pRPL0->getDeltaPocMSBPresentFlag(i), m_pcPic->layerId);
      refPicPOC = pcRefPic->getPOC();
    }
    refPicDecodingOrderNumber = pcRefPic->getDecodingOrderNumber();

    // Checking this: "When the current picture is a CRA picture, there shall be no entry in RefPicList[0] or RefPicList[1]
    // that precedes, in output order or decoding order, any preceding IRAP picture in decoding order (when present)"
    if (m_eNalUnitType == NAL_UNIT_CODED_SLICE_CRA)
    {
      CHECK(refPicPOC < irapPOC || refPicDecodingOrderNumber < associatedIRAPDecodingOrderNumber, "CRA picture detected that violate the rule that no entry in RefPicList[] shall precede, in output order or decoding order, any preceding IRAP picture in decoding order (when present).");
    }

    // Checking this: "When the current picture is a trailing picture that follows in both decoding orderand output order one
    // or more leading pictures associated with the same IRAP picture, if any, there shall be no picture referred to by an
    // entry in RefPicList[0] or RefPicList[1] that precedes the associated IRAP picture in output order or decoding order"
    // Note that when not in field coding, we know that all leading pictures of an IRAP precedes all trailing pictures of the
    // same IRAP picture.
    if (currentPictureIsTrailing && !fieldSeqFlag) // 
    {
      CHECK(refPicPOC < irapPOC || refPicDecodingOrderNumber < associatedIRAPDecodingOrderNumber, "Trailing picture detected that follows one or more leading pictures, if any, and violates the rule that no entry in RefPicList[] shall precede the associated IRAP picture in output order or decoding order.");
    }

    if (i < numActiveEntriesL0)
    {
      // Checking this "When the current picture is a trailing picture, there shall be no picture referred to by an active
      // entry in RefPicList[ 0 ] or RefPicList[ 1 ] that precedes the associated IRAP picture in output order or decoding order"
      if (currentPictureIsTrailing)
      {
        CHECK(refPicPOC < irapPOC || refPicDecodingOrderNumber < associatedIRAPDecodingOrderNumber, "Trailing picture detected that violate the rule that no active entry in RefPicList[] shall precede the associated IRAP picture in output order or decoding order");
      }

      // Checking this: "When the current picture is a RADL picture, there shall be no active entry in RefPicList[ 0 ] or
      // RefPicList[ 1 ] that is any of the following: A picture that precedes the associated IRAP picture in decoding order"
      if (m_eNalUnitType == NAL_UNIT_CODED_SLICE_RADL)
      {
        CHECK(refPicDecodingOrderNumber < associatedIRAPDecodingOrderNumber, "RADL picture detected that violate the rule that no active entry in RefPicList[] shall precede the associated IRAP picture in decoding order");
      }
    }
  }

  for (int i = 0; i < numEntriesL1; i++)
  {
    if (!pRPL1->isRefPicLongterm(i))
    {
      refPicPOC = getPOC() - pRPL1->getRefPicIdentifier(i);
      pcRefPic = xGetRefPic(rcListPic, refPicPOC, m_pcPic->layerId);
    }
    else
    {
      int pocBits = getSPS()->getBitsForPOC();
      int pocMask = (1 << pocBits) - 1;
      int ltrpPoc = pRPL1->getRefPicIdentifier(i) & pocMask;
      if(pRPL1->getDeltaPocMSBPresentFlag(i))
      {
        ltrpPoc += getPOC() - pRPL1->getDeltaPocMSBCycleLT(i) * (pocMask + 1) - (getPOC() & pocMask);
      }
      pcRefPic = xGetLongTermRefPic(rcListPic, ltrpPoc, pRPL1->getDeltaPocMSBPresentFlag(i), m_pcPic->layerId);
      refPicPOC = pcRefPic->getPOC();
    }
    refPicDecodingOrderNumber = pcRefPic->getDecodingOrderNumber();

    if (m_eNalUnitType == NAL_UNIT_CODED_SLICE_CRA)
    {
      CHECK(refPicPOC < irapPOC || refPicDecodingOrderNumber < associatedIRAPDecodingOrderNumber, "CRA picture detected that violate the rule that no entry in RefPicList[] shall precede, in output order or decoding order, any preceding IRAP picture in decoding order (when present).");
    }
    if (currentPictureIsTrailing && !fieldSeqFlag)
    {
      CHECK(refPicPOC < irapPOC || refPicDecodingOrderNumber < associatedIRAPDecodingOrderNumber, "Trailing picture detected that follows one or more leading pictures, if any, and violates the rule that no entry in RefPicList[] shall precede the associated IRAP picture in output order or decoding order.");
    }

    if (i < numActiveEntriesL1)
    {
      if (currentPictureIsTrailing)
      {
        CHECK(refPicPOC < irapPOC || refPicDecodingOrderNumber < associatedIRAPDecodingOrderNumber, "Trailing picture detected that violate the rule that no active entry in RefPicList[] shall precede the associated IRAP picture in output order or decoding order");
      }
      if (m_eNalUnitType == NAL_UNIT_CODED_SLICE_RADL)
      {
        CHECK(refPicDecodingOrderNumber < associatedIRAPDecodingOrderNumber, "RADL picture detected that violate the rule that no active entry in RefPicList[] shall precede the associated IRAP picture in decoding order");
      }
    }
  }
}
#endif


void Slice::checkSTSA(PicList& rcListPic)
{
  int ii;
  Picture* pcRefPic = NULL;
  int numOfActiveRef = getNumRefIdx(REF_PIC_LIST_0);

  for (ii = 0; ii < numOfActiveRef; ii++)
  {
    pcRefPic = m_apcRefPicList[REF_PIC_LIST_0][ii];

#if JVET_Q0156_STSA
    if( m_eNalUnitType == NAL_UNIT_CODED_SLICE_STSA && pcRefPic->layerId == m_pcPic->layerId )
    {
      CHECK( pcRefPic->layer == m_uiTLayer, "When the current picture is an STSA picture and nuh_layer_id equal to that of the current picture, there shall be no active entry in the RPL that has TemporalId equal to that of the current picture" );
    }
#else
    // Checking this: "When the current picture is an STSA picture, there shall be no active entry in RefPicList[ 0 ] or RefPicList[ 1 ] that has TemporalId equal to that of the current picture"
    if (getNalUnitType() == NAL_UNIT_CODED_SLICE_STSA)
    {
      CHECK(pcRefPic->layer == m_uiTLayer, "When the current picture is an STSA picture, there shall be no active entry in the RPL that has TemporalId equal to that of the current picture");
    }
#endif

    // Checking this: "When the current picture is a picture that follows, in decoding order, an STSA picture that has TemporalId equal to that of the current picture, there shall be no
    // picture that has TemporalId equal to that of the current picture included as an active entry in RefPicList[ 0 ] or RefPicList[ 1 ] that precedes the STSA picture in decoding order."
    CHECK(pcRefPic->subLayerNonReferencePictureDueToSTSA, "The RPL of the current picture contains a picture that is not allowed in this temporal layer due to an earlier STSA picture");
  }

  numOfActiveRef = getNumRefIdx(REF_PIC_LIST_1);
  for (ii = 0; ii < numOfActiveRef; ii++)
  {
    pcRefPic = m_apcRefPicList[REF_PIC_LIST_1][ii];

#if JVET_Q0156_STSA
    if( m_eNalUnitType == NAL_UNIT_CODED_SLICE_STSA && pcRefPic->layerId == m_pcPic->layerId )
    {
      CHECK( pcRefPic->layer == m_uiTLayer, "When the current picture is an STSA picture and nuh_layer_id equal to that of the current picture, there shall be no active entry in the RPL that has TemporalId equal to that of the current picture" );
    }
#else
    // Checking this: "When the current picture is an STSA picture, there shall be no active entry in RefPicList[ 0 ] or RefPicList[ 1 ] that has TemporalId equal to that of the current picture"
    if (getNalUnitType() == NAL_UNIT_CODED_SLICE_STSA)
    {
      CHECK(pcRefPic->layer == m_uiTLayer, "When the current picture is an STSA picture, there shall be no active entry in the RPL that has TemporalId equal to that of the current picture");
    }
#endif

    // Checking this: "When the current picture is a picture that follows, in decoding order, an STSA picture that has TemporalId equal to that of the current picture, there shall be no
    // picture that has TemporalId equal to that of the current picture included as an active entry in RefPicList[ 0 ] or RefPicList[ 1 ] that precedes the STSA picture in decoding order."
    CHECK(pcRefPic->subLayerNonReferencePictureDueToSTSA, "The active RPL part of the current picture contains a picture that is not allowed in this temporal layer due to an earlier STSA picture");
  }

  // If the current picture is an STSA picture, make all reference pictures in the DPB with temporal
  // id equal to the temproal id of the current picture sub-layer non-reference pictures. The flag
  // subLayerNonReferencePictureDueToSTSA equal to true means that the picture may not be used for
  // reference by a picture that follows the current STSA picture in decoding order
  if (getNalUnitType() == NAL_UNIT_CODED_SLICE_STSA)
  {
    PicList::iterator iterPic = rcListPic.begin();
    while (iterPic != rcListPic.end())
    {
      pcRefPic = *(iterPic++);
      if (!pcRefPic->referenced || pcRefPic->getPOC() == m_iPOC)
      {
        continue;
      }

      if (pcRefPic->layer == m_uiTLayer)
      {
        pcRefPic->subLayerNonReferencePictureDueToSTSA = true;
      }
    }
  }
}


/** Function for marking the reference pictures when an IDR/CRA/CRANT/BLA/BLANT is encountered.
 * \param pocCRA POC of the CRA/CRANT/BLA/BLANT picture
 * \param bRefreshPending flag indicating if a deferred decoding refresh is pending
 * \param rcListPic reference to the reference picture list
 * This function marks the reference pictures as "unused for reference" in the following conditions.
 * If the nal_unit_type is IDR/BLA/BLANT, all pictures in the reference picture list
 * are marked as "unused for reference"
 *    If the nal_unit_type is BLA/BLANT, set the pocCRA to the temporal reference of the current picture.
 * Otherwise
 *    If the bRefreshPending flag is true (a deferred decoding refresh is pending) and the current
 *    temporal reference is greater than the temporal reference of the latest CRA/CRANT/BLA/BLANT picture (pocCRA),
 *    mark all reference pictures except the latest CRA/CRANT/BLA/BLANT picture as "unused for reference" and set
 *    the bRefreshPending flag to false.
 *    If the nal_unit_type is CRA/CRANT, set the bRefreshPending flag to true and pocCRA to the temporal
 *    reference of the current picture.
 * Note that the current picture is already placed in the reference list and its marking is not changed.
 * If the current picture has a nal_ref_idc that is not 0, it will remain marked as "used for reference".
 */
void Slice::decodingRefreshMarking(int& pocCRA, bool& bRefreshPending, PicList& rcListPic, const bool bEfficientFieldIRAPEnabled)
{
  Picture* rpcPic;
  int      pocCurr = getPOC();

  if ( getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_W_RADL
    || getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_N_LP)  // IDR picture
  {
    // mark all pictures as not used for reference
    PicList::iterator        iterPic       = rcListPic.begin();
    while (iterPic != rcListPic.end())
    {
      rpcPic = *(iterPic);
      if (rpcPic->getPOC() != pocCurr)
      {
        rpcPic->referenced = false;
        rpcPic->getHashMap()->clearAll();
      }
      iterPic++;
    }
    if (bEfficientFieldIRAPEnabled)
    {
      bRefreshPending = true;
    }
  }
  else // CRA or No DR
  {
    if(bEfficientFieldIRAPEnabled && (getAssociatedIRAPType() == NAL_UNIT_CODED_SLICE_IDR_N_LP || getAssociatedIRAPType() == NAL_UNIT_CODED_SLICE_IDR_W_RADL))
    {
      if (bRefreshPending==true && pocCurr > m_iLastIDR) // IDR reference marking pending
      {
        PicList::iterator        iterPic       = rcListPic.begin();
        while (iterPic != rcListPic.end())
        {
          rpcPic = *(iterPic);
          if (rpcPic->getPOC() != pocCurr && rpcPic->getPOC() != m_iLastIDR)
          {
            rpcPic->referenced = false;
            rpcPic->getHashMap()->clearAll();
          }
          iterPic++;
        }
        bRefreshPending = false;
      }
    }
    else
    {
      if (bRefreshPending==true && pocCurr > pocCRA) // CRA reference marking pending
      {
        PicList::iterator iterPic = rcListPic.begin();
        while (iterPic != rcListPic.end())
        {
          rpcPic = *(iterPic);
          if (rpcPic->getPOC() != pocCurr && rpcPic->getPOC() != pocCRA)
          {
            rpcPic->referenced = false;
            rpcPic->getHashMap()->clearAll();
          }
          iterPic++;
        }
        bRefreshPending = false;
      }
    }
    if ( getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA ) // CRA picture found
    {
      bRefreshPending = true;
      pocCRA = pocCurr;
    }
  }
}

void Slice::copySliceInfo(Slice *pSrc, bool cpyAlmostAll)
{
  CHECK(!pSrc, "Source is NULL");

  int i, j, k;

  m_iPOC                 = pSrc->m_iPOC;
  m_eNalUnitType         = pSrc->m_eNalUnitType;
  m_eSliceType           = pSrc->m_eSliceType;
  m_iSliceQp             = pSrc->m_iSliceQp;
  m_iSliceQpBase         = pSrc->m_iSliceQpBase;
  m_ChromaQpAdjEnabled              = pSrc->m_ChromaQpAdjEnabled;
  m_deblockingFilterDisable         = pSrc->m_deblockingFilterDisable;
  m_deblockingFilterOverrideFlag    = pSrc->m_deblockingFilterOverrideFlag;
  m_deblockingFilterBetaOffsetDiv2  = pSrc->m_deblockingFilterBetaOffsetDiv2;
  m_deblockingFilterTcOffsetDiv2    = pSrc->m_deblockingFilterTcOffsetDiv2;
#if JVET_Q0121_DEBLOCKING_CONTROL_PARAMETERS
  m_deblockingFilterCbBetaOffsetDiv2  = pSrc->m_deblockingFilterCbBetaOffsetDiv2;
  m_deblockingFilterCbTcOffsetDiv2    = pSrc->m_deblockingFilterCbTcOffsetDiv2;
  m_deblockingFilterCrBetaOffsetDiv2  = pSrc->m_deblockingFilterCrBetaOffsetDiv2;
  m_deblockingFilterCrTcOffsetDiv2    = pSrc->m_deblockingFilterCrTcOffsetDiv2;
#endif
#if JVET_Q0089_SLICE_LOSSLESS_CODING_CHROMA_BDPCM
  m_tsResidualCodingDisabledFlag      = pSrc->m_tsResidualCodingDisabledFlag;
#endif

  for (i = 0; i < NUM_REF_PIC_LIST_01; i++)
  {
    m_aiNumRefIdx[i]     = pSrc->m_aiNumRefIdx[i];
  }

  for (i = 0; i < MAX_NUM_REF; i++)
  {
    m_list1IdxToList0Idx[i] = pSrc->m_list1IdxToList0Idx[i];
  }

  m_bCheckLDC             = pSrc->m_bCheckLDC;
  m_iSliceQpDelta        = pSrc->m_iSliceQpDelta;

  m_biDirPred = pSrc->m_biDirPred;
  m_symRefIdx[0] = pSrc->m_symRefIdx[0];
  m_symRefIdx[1] = pSrc->m_symRefIdx[1];

  for (uint32_t component = 0; component < MAX_NUM_COMPONENT; component++)
  {
    m_iSliceChromaQpDelta[component] = pSrc->m_iSliceChromaQpDelta[component];
  }
  m_iSliceChromaQpDelta[JOINT_CbCr] = pSrc->m_iSliceChromaQpDelta[JOINT_CbCr];

  for (i = 0; i < NUM_REF_PIC_LIST_01; i++)
  {
    for (j = 0; j < MAX_NUM_REF; j++)
    {
      m_apcRefPicList[i][j]  = pSrc->m_apcRefPicList[i][j];
      m_aiRefPOCList[i][j]   = pSrc->m_aiRefPOCList[i][j];
      m_bIsUsedAsLongTerm[i][j] = pSrc->m_bIsUsedAsLongTerm[i][j];
    }
    m_bIsUsedAsLongTerm[i][MAX_NUM_REF] = pSrc->m_bIsUsedAsLongTerm[i][MAX_NUM_REF];
  }
  if( cpyAlmostAll ) m_iDepth = pSrc->m_iDepth;

  // access channel
  if (cpyAlmostAll) m_pRPL0 = pSrc->m_pRPL0;
  if (cpyAlmostAll) m_pRPL1 = pSrc->m_pRPL1;
  m_iLastIDR             = pSrc->m_iLastIDR;

  if( cpyAlmostAll ) m_pcPic  = pSrc->m_pcPic;

  m_pcPicHeader          = pSrc->m_pcPicHeader;
  m_colFromL0Flag        = pSrc->m_colFromL0Flag;
  m_colRefIdx            = pSrc->m_colRefIdx;

  if( cpyAlmostAll ) setLambdas(pSrc->getLambdas());

  for (i = 0; i < NUM_REF_PIC_LIST_01; i++)
  {
    for (j = 0; j < MAX_NUM_REF; j++)
    {
      for (k =0; k < MAX_NUM_REF; k++)
      {
        m_abEqualRef[i][j][k] = pSrc->m_abEqualRef[i][j][k];
      }
    }
  }

  m_uiTLayer                      = pSrc->m_uiTLayer;
  m_bTLayerSwitchingFlag          = pSrc->m_bTLayerSwitchingFlag;

  m_sliceMap                      = pSrc->m_sliceMap;
  m_independentSliceIdx           = pSrc->m_independentSliceIdx;
  m_nextSlice                     = pSrc->m_nextSlice;
  m_clpRngs                       = pSrc->m_clpRngs;
#if JVET_Q0346_LMCS_ENABLE_IN_SH
  m_lmcsEnabledFlag               = pSrc->m_lmcsEnabledFlag;
#endif
#if JVET_Q0346_SCALING_LIST_USED_IN_SH
  m_explicitScalingListUsed       = pSrc->m_explicitScalingListUsed;
#endif

  m_pendingRasInit                = pSrc->m_pendingRasInit;

  for ( uint32_t e=0 ; e<NUM_REF_PIC_LIST_01 ; e++ )
  {
    for ( uint32_t n=0 ; n<MAX_NUM_REF ; n++ )
    {
      memcpy(m_weightPredTable[e][n], pSrc->m_weightPredTable[e][n], sizeof(WPScalingParam)*MAX_NUM_COMPONENT );
    }
  }

  for( uint32_t ch = 0 ; ch < MAX_NUM_CHANNEL_TYPE; ch++)
  {
    m_saoEnabledFlag[ch] = pSrc->m_saoEnabledFlag[ch];
  }

  m_cabacInitFlag                 = pSrc->m_cabacInitFlag;
  memcpy(m_alfApss, pSrc->m_alfApss, sizeof(m_alfApss)); // this might be quite unsafe
  memcpy( m_tileGroupAlfEnabledFlag, pSrc->m_tileGroupAlfEnabledFlag, sizeof(m_tileGroupAlfEnabledFlag));
  m_tileGroupNumAps               = pSrc->m_tileGroupNumAps;
  m_tileGroupLumaApsId            = pSrc->m_tileGroupLumaApsId;
  m_tileGroupChromaApsId          = pSrc->m_tileGroupChromaApsId;
  m_disableSATDForRd              = pSrc->m_disableSATDForRd;

  if( cpyAlmostAll ) m_encCABACTableIdx  = pSrc->m_encCABACTableIdx;
  for( int i = 0; i < NUM_REF_PIC_LIST_01; i ++ )
  {
    for (int j = 0; j < MAX_NUM_REF_PICS; j ++ )
    {
      m_scalingRatio[i][j]          = pSrc->m_scalingRatio[i][j];
    }
  }
#if JVET_Q0795_CCALF
  m_ccAlfFilterParam                        = pSrc->m_ccAlfFilterParam;
  m_ccAlfFilterControl[0]                   = pSrc->m_ccAlfFilterControl[0];
  m_ccAlfFilterControl[1]                   = pSrc->m_ccAlfFilterControl[1];
  m_tileGroupCcAlfCbEnabledFlag             = pSrc->m_tileGroupCcAlfCbEnabledFlag;
  m_tileGroupCcAlfCrEnabledFlag             = pSrc->m_tileGroupCcAlfCrEnabledFlag;
  m_tileGroupCcAlfCbApsId                   = pSrc->m_tileGroupCcAlfCbApsId;
  m_tileGroupCcAlfCrApsId                   = pSrc->m_tileGroupCcAlfCrApsId;
#endif
}


/** Function for checking if this is a switching-point
*/
bool Slice::isTemporalLayerSwitchingPoint(PicList& rcListPic) const
{
  // loop through all pictures in the reference picture buffer
  PicList::iterator iterPic = rcListPic.begin();
  while ( iterPic != rcListPic.end())
  {
    const Picture* pcPic = *(iterPic++);
    if( pcPic->referenced && pcPic->poc != getPOC())
    {
      if( pcPic->layer >= getTLayer())
      {
        return false;
      }
    }
  }
  return true;
}

/** Function for checking if this is a STSA candidate
 */
bool Slice::isStepwiseTemporalLayerSwitchingPointCandidate(PicList& rcListPic) const
{
  PicList::iterator iterPic = rcListPic.begin();
  while ( iterPic != rcListPic.end())
  {
    const Picture* pcPic = *(iterPic++);
    if( pcPic->referenced && pcPic->poc != getPOC())
    {
      if( pcPic->layer >= getTLayer())
      {
        return false;
      }
    }
  }
  return true;
}


#if JVET_Q0751_MIXED_NAL_UNIT_TYPES
void Slice::checkLeadingPictureRestrictions(PicList& rcListPic, const PPS& pps) const
#else
void Slice::checkLeadingPictureRestrictions(PicList& rcListPic) const
#endif
{
  int nalUnitType = this->getNalUnitType();

  // When a picture is a leading picture, it shall be a RADL or RASL picture.
  if(this->getAssociatedIRAPPOC() > this->getPOC())
  {
#if JVET_Q0751_MIXED_NAL_UNIT_TYPES
    //check this only when mixed_nalu_types_in_pic_flag is equal to 0
    if (pps.getMixedNaluTypesInPicFlag() == 0)
    {
      // Do not check IRAP pictures since they may get a POC lower than their associated IRAP
      if (nalUnitType < NAL_UNIT_CODED_SLICE_IDR_W_RADL ||
          nalUnitType > NAL_UNIT_CODED_SLICE_CRA)
      {
        CHECK(nalUnitType != NAL_UNIT_CODED_SLICE_RASL &&
              nalUnitType != NAL_UNIT_CODED_SLICE_RADL, "Invalid NAL unit type");
      }
    }
#else
    // Do not check IRAP pictures since they may get a POC lower than their associated IRAP
    if (nalUnitType < NAL_UNIT_CODED_SLICE_IDR_W_RADL ||
        nalUnitType > NAL_UNIT_CODED_SLICE_CRA)
    {
      CHECK(nalUnitType != NAL_UNIT_CODED_SLICE_RASL &&
            nalUnitType != NAL_UNIT_CODED_SLICE_RADL, "Invalid NAL unit type");
    }
#endif
  }

  // When a picture is a trailing picture, it shall not be a RADL or RASL picture.
  if(this->getAssociatedIRAPPOC() < this->getPOC())
  {
#if JVET_Q0751_MIXED_NAL_UNIT_TYPES
      //check this only when mixed_nalu_types_in_pic_flag is equal to 0
    if (pps.getMixedNaluTypesInPicFlag() == 0)
    {
      CHECK(nalUnitType == NAL_UNIT_CODED_SLICE_RASL ||
            nalUnitType == NAL_UNIT_CODED_SLICE_RADL, "Invalid NAL unit type");
    }
#else
    CHECK(nalUnitType == NAL_UNIT_CODED_SLICE_RASL ||
          nalUnitType == NAL_UNIT_CODED_SLICE_RADL, "Invalid NAL unit type");
#endif

  }


  // No RASL pictures shall be present in the bitstream that are associated with
  // an IDR picture.
  if (nalUnitType == NAL_UNIT_CODED_SLICE_RASL)
  {
    CHECK( this->getAssociatedIRAPType() == NAL_UNIT_CODED_SLICE_IDR_N_LP   ||
           this->getAssociatedIRAPType() == NAL_UNIT_CODED_SLICE_IDR_W_RADL, "Invalid NAL unit type");
  }

  // No RADL pictures shall be present in the bitstream that are associated with
  // a BLA picture having nal_unit_type equal to BLA_N_LP or that are associated
  // with an IDR picture having nal_unit_type equal to IDR_N_LP.
  if (nalUnitType == NAL_UNIT_CODED_SLICE_RADL)
  {
    CHECK (this->getAssociatedIRAPType() == NAL_UNIT_CODED_SLICE_IDR_N_LP, "Invalid NAL unit type");
  }

  // loop through all pictures in the reference picture buffer
  PicList::iterator iterPic = rcListPic.begin();
  int numLeadingPicsFound = 0;
  while ( iterPic != rcListPic.end())
  {
    Picture* pcPic = *(iterPic++);
    if( ! pcPic->reconstructed)
    {
      continue;
    }
    if( pcPic->poc == this->getPOC())
    {
      continue;
    }
    const Slice* pcSlice = pcPic->slices[0];

    // Any picture that has PicOutputFlag equal to 1 that precedes an IRAP picture
    // in decoding order shall precede the IRAP picture in output order.
    // (Note that any picture following in output order would be present in the DPB)
    if(pcSlice->getPicHeader()->getPicOutputFlag() == 1 && !this->getPicHeader()->getNoOutputOfPriorPicsFlag())
    {
      if (nalUnitType == NAL_UNIT_CODED_SLICE_CRA ||
          nalUnitType == NAL_UNIT_CODED_SLICE_IDR_N_LP ||
          nalUnitType == NAL_UNIT_CODED_SLICE_IDR_W_RADL)
      {
        CHECK(pcPic->poc >= this->getPOC(), "Invalid POC");
      }
    }

    // Any picture that has PicOutputFlag equal to 1 that precedes an IRAP picture
    // in decoding order shall precede any RADL picture associated with the IRAP
    // picture in output order.
    if(pcSlice->getPicHeader()->getPicOutputFlag() == 1)
    {
      if (nalUnitType == NAL_UNIT_CODED_SLICE_RADL)
      {
        // rpcPic precedes the IRAP in decoding order
        if(this->getAssociatedIRAPPOC() > pcSlice->getAssociatedIRAPPOC())
        {
          // rpcPic must not be the IRAP picture
          if(this->getAssociatedIRAPPOC() != pcPic->poc)
          {
            CHECK( pcPic->poc >= this->getPOC(), "Invalid POC");
          }
        }
      }
    }

    // When a picture is a leading picture, it shall precede, in decoding order,
    // all trailing pictures that are associated with the same IRAP picture.
    if ((nalUnitType == NAL_UNIT_CODED_SLICE_RASL || nalUnitType == NAL_UNIT_CODED_SLICE_RADL) &&
        (pcSlice->getNalUnitType() != NAL_UNIT_CODED_SLICE_RASL && pcSlice->getNalUnitType() != NAL_UNIT_CODED_SLICE_RADL)  )
    {
      if (pcSlice->getAssociatedIRAPPOC() == this->getAssociatedIRAPPOC())
      {
        numLeadingPicsFound++;
        int limitNonLP = 0;
#if JVET_Q0042_VUI
        if (pcSlice->getSPS()->getFieldSeqFlag())
#else
        if (pcSlice->getSPS()->getVuiParameters() && pcSlice->getSPS()->getVuiParameters()->getFieldSeqFlag())
#endif
          limitNonLP = 1;
        CHECK(pcPic->poc > this->getAssociatedIRAPPOC() && numLeadingPicsFound > limitNonLP, "Invalid POC");
      }
    }

    // Any RASL picture associated with a CRA or BLA picture shall precede any
    // RADL picture associated with the CRA or BLA picture in output order
    if (nalUnitType == NAL_UNIT_CODED_SLICE_RASL)
    {
      if ((this->getAssociatedIRAPType() == NAL_UNIT_CODED_SLICE_CRA) &&
          this->getAssociatedIRAPPOC() == pcSlice->getAssociatedIRAPPOC())
      {
        if (pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_RADL)
        {
          CHECK( pcPic->poc <= this->getPOC(), "Invalid POC");
        }
      }
    }

    // Any RASL picture associated with a CRA picture shall follow, in output
    // order, any IRAP picture that precedes the CRA picture in decoding order.
    if (nalUnitType == NAL_UNIT_CODED_SLICE_RASL)
    {
      if(this->getAssociatedIRAPType() == NAL_UNIT_CODED_SLICE_CRA)
      {
        if(pcSlice->getPOC() < this->getAssociatedIRAPPOC() &&
          (
            pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_N_LP   ||
            pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_W_RADL ||
            pcSlice->getNalUnitType() == NAL_UNIT_CODED_SLICE_CRA))
        {
          CHECK(this->getPOC() <= pcSlice->getPOC(), "Invalid POC");
        }
      }
    }
  }
}




//Function for applying picture marking based on the Reference Picture List
#if JVET_Q0751_MIXED_NAL_UNIT_TYPES
void Slice::applyReferencePictureListBasedMarking( PicList& rcListPic, const ReferencePictureList *pRPL0, const ReferencePictureList *pRPL1, const int layerId, const PPS& pps ) const
#else
void Slice::applyReferencePictureListBasedMarking( PicList& rcListPic, const ReferencePictureList *pRPL0, const ReferencePictureList *pRPL1, const int layerId ) const
#endif
{
  int i, isReference;
#if JVET_Q0751_MIXED_NAL_UNIT_TYPES
  checkLeadingPictureRestrictions(rcListPic, pps);
#else
  checkLeadingPictureRestrictions(rcListPic);
#endif

  bool isNeedToCheck = (this->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_N_LP || this->getNalUnitType() == NAL_UNIT_CODED_SLICE_IDR_W_RADL) ? false : true;

  // mark long-term reference pictures in List0
  for( i = 0; i < pRPL0->getNumberOfShorttermPictures() + pRPL0->getNumberOfLongtermPictures() + pRPL0->getNumberOfInterLayerPictures(); i++ )
  {
    if( !pRPL0->isRefPicLongterm( i ) || pRPL0->isInterLayerRefPic( i ) )
    {
      continue;
    }
          
    int isAvailable = 0;
    PicList::iterator iterPic = rcListPic.begin();
    while (iterPic != rcListPic.end())
    {
      Picture* rpcPic = *(iterPic++);
      if (!rpcPic->referenced)
      {
        continue;
      }
      int pocCycle = 1 << (rpcPic->cs->sps->getBitsForPOC());
      int curPoc = rpcPic->getPOC();
      int refPoc = pRPL0->getRefPicIdentifier(i) & (pocCycle - 1);
      if(pRPL0->getDeltaPocMSBPresentFlag(i))
      {
        refPoc += getPOC() - pRPL0->getDeltaPocMSBCycleLT(i) * pocCycle - (getPOC() & (pocCycle - 1));
      }
      else
      {
        curPoc = curPoc & (pocCycle - 1);
      }
      if (rpcPic->longTerm && curPoc == refPoc && rpcPic->referenced)
      {
        isAvailable = 1;
        break;
      }
    }
    // if there was no such long-term check the short terms
    if (!isAvailable)
    {
      iterPic = rcListPic.begin();
      while (iterPic != rcListPic.end())
      {
        Picture* rpcPic = *(iterPic++);
        if (!rpcPic->referenced)
        {
          continue;
        }
        int pocCycle = 1 << (rpcPic->cs->sps->getBitsForPOC());
        int curPoc = rpcPic->getPOC();
        int refPoc = pRPL0->getRefPicIdentifier(i) & (pocCycle - 1);
        if(pRPL0->getDeltaPocMSBPresentFlag(i))
        {
          refPoc += getPOC() - pRPL0->getDeltaPocMSBCycleLT(i) * pocCycle - (getPOC() & (pocCycle - 1));
        }
        else
        {
          curPoc = curPoc & (pocCycle - 1);
        }
        if (!rpcPic->longTerm && curPoc == refPoc && rpcPic->referenced)
        {
          isAvailable = 1;
          rpcPic->longTerm = true;
          break;
        }
      }
    }
  }

  // mark long-term reference pictures in List1
  for( i = 0; i < pRPL1->getNumberOfShorttermPictures() + pRPL1->getNumberOfLongtermPictures() + pRPL1->getNumberOfInterLayerPictures(); i++ )
  {
    if( !pRPL1->isRefPicLongterm( i ) || pRPL1->isInterLayerRefPic( i ) )
    {
      continue;
    }
          
    int isAvailable = 0;
    PicList::iterator iterPic = rcListPic.begin();
    while (iterPic != rcListPic.end())
    {
      Picture* rpcPic = *(iterPic++);
      if (!rpcPic->referenced)
      {
        continue;
      }
      int pocCycle = 1 << (rpcPic->cs->sps->getBitsForPOC());
      int curPoc = rpcPic->getPOC();
      int refPoc = pRPL1->getRefPicIdentifier(i) & (pocCycle - 1);
      if(pRPL1->getDeltaPocMSBPresentFlag(i))
      {
        refPoc += getPOC() - pRPL1->getDeltaPocMSBCycleLT(i) * pocCycle - (getPOC() & (pocCycle - 1));
      }
      else
      {
        curPoc = curPoc & (pocCycle - 1);
      }
      if (rpcPic->longTerm && curPoc == refPoc && rpcPic->referenced)
      {
        isAvailable = 1;
        break;
      }
    }
    // if there was no such long-term check the short terms
    if (!isAvailable)
    {
      iterPic = rcListPic.begin();
      while (iterPic != rcListPic.end())
      {
        Picture* rpcPic = *(iterPic++);
        if (!rpcPic->referenced)
        {
          continue;
        }
        int pocCycle = 1 << (rpcPic->cs->sps->getBitsForPOC());
        int curPoc = rpcPic->getPOC();
        int refPoc = pRPL1->getRefPicIdentifier(i) & (pocCycle - 1);
        if(pRPL1->getDeltaPocMSBPresentFlag(i))
        {
          refPoc += getPOC() - pRPL1->getDeltaPocMSBCycleLT(i) * pocCycle - (getPOC() & (pocCycle - 1));
        }
        else
        {
          curPoc = curPoc & (pocCycle - 1);
        }
        if (!rpcPic->longTerm && curPoc == refPoc && rpcPic->referenced)
        {
          isAvailable = 1;
          rpcPic->longTerm = true;
          break;
        }
      }
    }
  }

  // loop through all pictures in the reference picture buffer
  PicList::iterator iterPic = rcListPic.begin();
  while (iterPic != rcListPic.end())
  {
    Picture* pcPic = *(iterPic++);

    if (!pcPic->referenced)
      continue;

    isReference = 0;
    // loop through all pictures in the Reference Picture Set
    // to see if the picture should be kept as reference picture
    for( i = 0; isNeedToCheck && !isReference && i < pRPL0->getNumberOfShorttermPictures() + pRPL0->getNumberOfLongtermPictures() + pRPL0->getNumberOfInterLayerPictures(); i++ )
    {
      if( pRPL0->isInterLayerRefPic( i ) )
      {
        // Diagonal inter-layer prediction is not allowed
        CHECK( pRPL0->getRefPicIdentifier( i ), "ILRP identifier should be 0" );

        if( pcPic->poc == m_iPOC )
        {
          isReference = 1;
          pcPic->longTerm = true;
        }
      }
      else if (pcPic->layerId == layerId)
      {
      if (!(pRPL0->isRefPicLongterm(i)))
      {
        if (pcPic->poc == this->getPOC() - pRPL0->getRefPicIdentifier(i))
        {
          isReference = 1;
          pcPic->longTerm = false;
        }
      }
      else
      {
        int pocCycle = 1 << (pcPic->cs->sps->getBitsForPOC());
        int curPoc = pcPic->poc;
        int refPoc = pRPL0->getRefPicIdentifier(i) & (pocCycle - 1);
        if(pRPL0->getDeltaPocMSBPresentFlag(i))
        {
          refPoc += getPOC() - pRPL0->getDeltaPocMSBCycleLT(i) * pocCycle - (getPOC() & (pocCycle - 1));
        }
        else
        {
          curPoc = curPoc & (pocCycle - 1);
        }
        if (pcPic->longTerm && curPoc == refPoc)
        {
          isReference = 1;
          pcPic->longTerm = true;
        }
      }
      }
    }

    for( i = 0; isNeedToCheck && !isReference && i < pRPL1->getNumberOfShorttermPictures() + pRPL1->getNumberOfLongtermPictures() + pRPL1->getNumberOfInterLayerPictures(); i++ )
    {
      if( pRPL1->isInterLayerRefPic( i ) )
      {
        // Diagonal inter-layer prediction is not allowed
        CHECK( pRPL1->getRefPicIdentifier( i ), "ILRP identifier should be 0" );

        if( pcPic->poc == m_iPOC )
        {
          isReference = 1;
          pcPic->longTerm = true;
        }
      }
      else if( pcPic->layerId == layerId )
      {
      if (!(pRPL1->isRefPicLongterm(i)))
      {
        if (pcPic->poc == this->getPOC() - pRPL1->getRefPicIdentifier(i))
        {
          isReference = 1;
          pcPic->longTerm = false;
        }
      }
      else
      {
        int pocCycle = 1 << (pcPic->cs->sps->getBitsForPOC());
        int curPoc = pcPic->poc;
        int refPoc = pRPL1->getRefPicIdentifier(i) & (pocCycle - 1);
        if(pRPL1->getDeltaPocMSBPresentFlag(i))
        {
          refPoc += getPOC() - pRPL1->getDeltaPocMSBCycleLT(i) * pocCycle - (getPOC() & (pocCycle - 1));
        }
        else
        {
          curPoc = curPoc & (pocCycle - 1);
        }
        if (pcPic->longTerm && curPoc == refPoc)
        {
          isReference = 1;
          pcPic->longTerm = true;
        }
      }
      }
    }
    // mark the picture as "unused for reference" if it is not in
    // the Reference Picture List
    if( pcPic->layerId == layerId && pcPic->poc != m_iPOC && isReference == 0 )
    {
      pcPic->referenced = false;
      pcPic->longTerm = false;
    }

    // sanity checks
    if (pcPic->referenced)
    {
      //check that pictures of higher temporal layers are not used
      CHECK(pcPic->usedByCurr && !(pcPic->layer <= this->getTLayer()), "Invalid state");
    }
  }
}

int Slice::checkThatAllRefPicsAreAvailable(PicList& rcListPic, const ReferencePictureList *pRPL, int rplIdx, bool printErrors) const
{
  Picture* rpcPic;
  int isAvailable = 0;
  int notPresentPoc = 0;

  if (this->isIDRorBLA()) return 0; //Assume that all pic in the DPB will be flushed anyway so no need to check.

  int numberOfPictures = pRPL->getNumberOfLongtermPictures() + pRPL->getNumberOfShorttermPictures() + pRPL->getNumberOfInterLayerPictures();
  //Check long term ref pics
  for (int ii = 0; pRPL->getNumberOfLongtermPictures() > 0 && ii < numberOfPictures; ii++)
  {
    if( !pRPL->isRefPicLongterm( ii ) || pRPL->isInterLayerRefPic( ii ) )
    {
      continue;
    }

    notPresentPoc = pRPL->getRefPicIdentifier(ii);
    isAvailable = 0;
    PicList::iterator iterPic = rcListPic.begin();
    while (iterPic != rcListPic.end())
    {
      rpcPic = *(iterPic++);
      int pocCycle = 1 << (rpcPic->cs->sps->getBitsForPOC());
      int curPoc = rpcPic->getPOC();
      int refPoc = pRPL->getRefPicIdentifier(ii) & (pocCycle - 1);
      if(pRPL->getDeltaPocMSBPresentFlag(ii))
      {
        refPoc += getPOC() - pRPL->getDeltaPocMSBCycleLT(ii) * pocCycle - (getPOC() & (pocCycle - 1));
      }
      else
      {
        curPoc = curPoc & (pocCycle - 1);
      }
      if (rpcPic->longTerm && curPoc == refPoc && rpcPic->referenced)
      {
        isAvailable = 1;
        break;
      }
    }
    // if there was no such long-term check the short terms
    if (!isAvailable)
    {
      iterPic = rcListPic.begin();
      while (iterPic != rcListPic.end())
      {
        rpcPic = *(iterPic++);
        int pocCycle = 1 << (rpcPic->cs->sps->getBitsForPOC());
        int curPoc = rpcPic->getPOC();
        int refPoc = pRPL->getRefPicIdentifier(ii) & (pocCycle - 1);
        if(pRPL->getDeltaPocMSBPresentFlag(ii))
        {
          refPoc += getPOC() - pRPL->getDeltaPocMSBCycleLT(ii) * pocCycle - (getPOC() & (pocCycle - 1));
        }
        else
        {
          curPoc = curPoc & (pocCycle - 1);
        }
        if (!rpcPic->longTerm && curPoc == refPoc && rpcPic->referenced)
        {
          isAvailable = 1;
          rpcPic->longTerm = true;
          break;
        }
      }
    }
    if (!isAvailable)
    {
      if (printErrors)
      {
        msg(ERROR, "\nCurrent picture: %d Long-term reference picture with POC = %3d seems to have been removed or not correctly decoded.", this->getPOC(), notPresentPoc);
      }
      return notPresentPoc;
    }
  }
  //report that a picture is lost if it is in the Reference Picture List but not in the DPB

  isAvailable = 0;
  //Check short term ref pics
  for (int ii = 0; ii < numberOfPictures; ii++)
  {
    if (pRPL->isRefPicLongterm(ii))
      continue;

    notPresentPoc = this->getPOC() - pRPL->getRefPicIdentifier(ii);
    isAvailable = 0;
    PicList::iterator iterPic = rcListPic.begin();
    while (iterPic != rcListPic.end())
    {
      rpcPic = *(iterPic++);
      if (!rpcPic->longTerm && rpcPic->getPOC() == this->getPOC() - pRPL->getRefPicIdentifier(ii) && rpcPic->referenced)
      {
        isAvailable = 1;
        break;
      }
    }
    //report that a picture is lost if it is in the Reference Picture List but not in the DPB
    if (isAvailable == 0 && pRPL->getNumberOfShorttermPictures() > 0)
    {
      if (printErrors)
      {
        msg(ERROR, "\nCurrent picture: %d Short-term reference picture with POC = %3d seems to have been removed or not correctly decoded.", this->getPOC(), notPresentPoc);
      }
      return notPresentPoc;
    }
  }
  return 0;
}

int Slice::checkThatAllRefPicsAreAvailable(PicList& rcListPic, const ReferencePictureList *pRPL, int rplIdx, bool printErrors, int *refPicIndex, int numActiveRefPics) const
{
  Picture* rpcPic;
  int isAvailable = 0;
  int notPresentPoc = 0;
  *refPicIndex = 0;

  if (this->isIDRorBLA()) return 0; //Assume that all pic in the DPB will be flushed anyway so no need to check.

  int numberOfPictures = numActiveRefPics;
  //Check long term ref pics
  for (int ii = 0; pRPL->getNumberOfLongtermPictures() > 0 && ii < numberOfPictures; ii++)
  {
    if( !pRPL->isRefPicLongterm( ii ) || pRPL->isInterLayerRefPic( ii ) )
    {
      continue;
    }

    notPresentPoc = pRPL->getRefPicIdentifier(ii);
    isAvailable = 0;
    PicList::iterator iterPic = rcListPic.begin();
    while (iterPic != rcListPic.end())
    {
      rpcPic = *(iterPic++);
      int pocCycle = 1 << (rpcPic->cs->sps->getBitsForPOC());
      int curPoc = rpcPic->getPOC();
      int refPoc = pRPL->getRefPicIdentifier(ii) & (pocCycle - 1);
      if(pRPL->getDeltaPocMSBPresentFlag(ii))
      {
        refPoc += getPOC() - pRPL->getDeltaPocMSBCycleLT(ii) * pocCycle - (getPOC() & (pocCycle - 1));
      }
      else
      {
        curPoc = curPoc & (pocCycle - 1);
      }
      if (rpcPic->longTerm && curPoc == refPoc && rpcPic->referenced)
      {
        isAvailable = 1;
        break;
      }
    }
    // if there was no such long-term check the short terms
    if (!isAvailable)
    {
      iterPic = rcListPic.begin();
      while (iterPic != rcListPic.end())
      {
        rpcPic = *(iterPic++);
        int pocCycle = 1 << (rpcPic->cs->sps->getBitsForPOC());
        int curPoc = rpcPic->getPOC();
        int refPoc = pRPL->getRefPicIdentifier(ii) & (pocCycle - 1);
        if(pRPL->getDeltaPocMSBPresentFlag(ii))
        {
          refPoc += getPOC() - pRPL->getDeltaPocMSBCycleLT(ii) * pocCycle - (getPOC() & (pocCycle - 1));
        }
        else
        {
          curPoc = curPoc & (pocCycle - 1);
        }
        if (!rpcPic->longTerm && curPoc == refPoc && rpcPic->referenced)
        {
          isAvailable = 1;
          rpcPic->longTerm = true;
          break;
        }
      }
    }
    if (!isAvailable)
    {
      if (printErrors)
      {
        msg(ERROR, "\nCurrent picture: %d Long-term reference picture with POC = %3d seems to have been removed or not correctly decoded.", this->getPOC(), notPresentPoc);
      }
      *refPicIndex = ii;
      return notPresentPoc;
    }
  }
  //report that a picture is lost if it is in the Reference Picture List but not in the DPB

  isAvailable = 0;
  //Check short term ref pics
  for (int ii = 0; ii < numberOfPictures; ii++)
  {
    if (pRPL->isRefPicLongterm(ii))
      continue;

    notPresentPoc = this->getPOC() - pRPL->getRefPicIdentifier(ii);
    isAvailable = 0;
    PicList::iterator iterPic = rcListPic.begin();
    while (iterPic != rcListPic.end())
    {
      rpcPic = *(iterPic++);
      if (!rpcPic->longTerm && rpcPic->getPOC() == this->getPOC() - pRPL->getRefPicIdentifier(ii) && rpcPic->referenced)
      {
        isAvailable = 1;
        break;
      }
    }
    //report that a picture is lost if it is in the Reference Picture List but not in the DPB
    if (isAvailable == 0 && pRPL->getNumberOfShorttermPictures() > 0)
    {
      if (printErrors)
      {
        msg(ERROR, "\nCurrent picture: %d Short-term reference picture with POC = %3d seems to have been removed or not correctly decoded.", this->getPOC(), notPresentPoc);
      }
      *refPicIndex = ii;
      return notPresentPoc;
    }
  }
  return 0;
}

bool Slice::isPOCInRefPicList(const ReferencePictureList *rpl, int poc )
{
  for( int i = 0; i < rpl->getNumberOfLongtermPictures() + rpl->getNumberOfShorttermPictures() + rpl->getNumberOfInterLayerPictures(); i++ )
  {
    if( rpl->isInterLayerRefPic( i ) )
    {
      // Diagonal inter-layer prediction is not allowed
      CHECK( rpl->getRefPicIdentifier( i ), "ILRP identifier should be 0" );

      if( poc == m_iPOC )
      {
        return true;
      }
    }
    else
    if (rpl->isRefPicLongterm(i))
    {
      if (poc == rpl->getRefPicIdentifier(i))
      {
        return true;
      }
    }
    else
    {
      if (poc == getPOC() - rpl->getRefPicIdentifier(i))
      {
        return true;
      }
    }
  }
  return false;
}

bool Slice::isPocRestrictedByDRAP( int poc, bool precedingDRAPInDecodingOrder )
{
  if (!getEnableDRAPSEI())
  {
    return false;
  }
  return ( isDRAP() && poc != getAssociatedIRAPPOC() ) ||
         ( cvsHasPreviousDRAP() && getPOC() > getLatestDRAPPOC() && (precedingDRAPInDecodingOrder || poc < getLatestDRAPPOC()) );
}

void Slice::checkConformanceForDRAP( uint32_t temporalId )
{
  if (!(isDRAP() || cvsHasPreviousDRAP()))
  {
    return;
  }

  if (isDRAP())
  {
    if (!(getNalUnitType() == NalUnitType::NAL_UNIT_CODED_SLICE_TRAIL ||
          getNalUnitType() == NalUnitType::NAL_UNIT_CODED_SLICE_STSA))
    {
      msg( WARNING, "Warning, non-conforming bitstream. The DRAP picture should be a trailing picture.\n");
    }
    if ( temporalId != 0)
    {
      msg( WARNING, "Warning, non-conforming bitstream. The DRAP picture shall have a temporal sublayer identifier equal to 0.\n");
    }
    for (int i = 0; i < getNumRefIdx(REF_PIC_LIST_0); i++)
    {
      if (getRefPic(REF_PIC_LIST_0,i)->getPOC() != getAssociatedIRAPPOC())
      {
        msg( WARNING, "Warning, non-conforming bitstream. The DRAP picture shall not include any pictures in the active "
                      "entries of its reference picture lists except the preceding IRAP picture in decoding order.\n");
      }
    }
    for (int i = 0; i < getNumRefIdx(REF_PIC_LIST_1); i++)
    {
      if (getRefPic(REF_PIC_LIST_1,i)->getPOC() != getAssociatedIRAPPOC())
      {
        msg( WARNING, "Warning, non-conforming bitstream. The DRAP picture shall not include any pictures in the active "
                      "entries of its reference picture lists except the preceding IRAP picture in decoding order.\n");
      }
    }
  }

  if (cvsHasPreviousDRAP() && getPOC() > getLatestDRAPPOC())
  {
    for (int i = 0; i < getNumRefIdx(REF_PIC_LIST_0); i++)
    {
      if (getRefPic(REF_PIC_LIST_0,i)->getPOC() < getLatestDRAPPOC() && getRefPic(REF_PIC_LIST_0,i)->getPOC() != getAssociatedIRAPPOC())
      {
        msg( WARNING, "Warning, non-conforming bitstream. Any picture that follows the DRAP picture in both decoding order "
                    "and output order shall not include, in the active entries of its reference picture lists, any picture "
                    "that precedes the DRAP picture in decoding order or output order, with the exception of the preceding "
                    "IRAP picture in decoding order. Problem is POC %d in RPL0.\n", getRefPic(REF_PIC_LIST_0,i)->getPOC());
      }
    }
    for (int i = 0; i < getNumRefIdx(REF_PIC_LIST_1); i++)
    {
      if (getRefPic(REF_PIC_LIST_1,i)->getPOC() < getLatestDRAPPOC() && getRefPic(REF_PIC_LIST_1,i)->getPOC() != getAssociatedIRAPPOC())
      {
        msg( WARNING, "Warning, non-conforming bitstream. Any picture that follows the DRAP picture in both decoding order "
                    "and output order shall not include, in the active entries of its reference picture lists, any picture "
                    "that precedes the DRAP picture in decoding order or output order, with the exception of the preceding "
                    "IRAP picture in decoding order. Problem is POC %d in RPL1", getRefPic(REF_PIC_LIST_1,i)->getPOC());
      }
    }
  }
}


//! get AC and DC values for weighted pred
void  Slice::getWpAcDcParam(const WPACDCParam *&wp) const
{
  wp = m_weightACDCParam;
}

//! init AC and DC values for weighted pred
void  Slice::initWpAcDcParam()
{
  for(int iComp = 0; iComp < MAX_NUM_COMPONENT; iComp++ )
  {
    m_weightACDCParam[iComp].iAC = 0;
    m_weightACDCParam[iComp].iDC = 0;
  }
}

//! get tables for weighted prediction
void  Slice::getWpScaling( RefPicList e, int iRefIdx, WPScalingParam *&wp ) const
{
  CHECK(e>=NUM_REF_PIC_LIST_01, "Invalid picture reference list");
  wp = (WPScalingParam*) m_weightPredTable[e][iRefIdx];
}

//! reset Default WP tables settings : no weight.
void  Slice::resetWpScaling()
{
  for ( int e=0 ; e<NUM_REF_PIC_LIST_01 ; e++ )
  {
    for ( int i=0 ; i<MAX_NUM_REF ; i++ )
    {
      for ( int yuv=0 ; yuv<MAX_NUM_COMPONENT ; yuv++ )
      {
        WPScalingParam  *pwp = &(m_weightPredTable[e][i][yuv]);
        pwp->bPresentFlag      = false;
        pwp->uiLog2WeightDenom = 0;
        pwp->uiLog2WeightDenom = 0;
        pwp->iWeight           = 1;
        pwp->iOffset           = 0;
      }
    }
  }
}

//! init WP table
void  Slice::initWpScaling(const SPS *sps)
{
  const bool bUseHighPrecisionPredictionWeighting = sps->getSpsRangeExtension().getHighPrecisionOffsetsEnabledFlag();
  for ( int e=0 ; e<NUM_REF_PIC_LIST_01 ; e++ )
  {
    for ( int i=0 ; i<MAX_NUM_REF ; i++ )
    {
      for ( int yuv=0 ; yuv<MAX_NUM_COMPONENT ; yuv++ )
      {
        WPScalingParam  *pwp = &(m_weightPredTable[e][i][yuv]);
        if ( !pwp->bPresentFlag )
        {
          // Inferring values not present :
          pwp->iWeight = (1 << pwp->uiLog2WeightDenom);
          pwp->iOffset = 0;
        }

        const int offsetScalingFactor = bUseHighPrecisionPredictionWeighting ? 1 : (1 << (sps->getBitDepth(toChannelType(ComponentID(yuv)))-8));

        pwp->w      = pwp->iWeight;
        pwp->o      = pwp->iOffset * offsetScalingFactor; //NOTE: This value of the ".o" variable is never used - .o is set immediately before it gets used
        pwp->shift  = pwp->uiLog2WeightDenom;
        pwp->round  = (pwp->uiLog2WeightDenom>=1) ? (1 << (pwp->uiLog2WeightDenom-1)) : (0);
      }
    }
  }
}


void Slice::startProcessingTimer()
{
  m_iProcessingStartTime = clock();
}

void Slice::stopProcessingTimer()
{
  m_dProcessingTime += (double)(clock()-m_iProcessingStartTime) / CLOCKS_PER_SEC;
  m_iProcessingStartTime = 0;
}

unsigned Slice::getMinPictureDistance() const
{
  int minPicDist = MAX_INT;
  if (getSPS()->getIBCFlag())
  {
    minPicDist = 0;
  }
  else
  if( ! isIntra() )
  {
    const int currPOC  = getPOC();
    for (int refIdx = 0; refIdx < getNumRefIdx(REF_PIC_LIST_0); refIdx++)
    {
      minPicDist = std::min( minPicDist, std::abs(currPOC - getRefPic(REF_PIC_LIST_0, refIdx)->getPOC()));
    }
    if( getSliceType() == B_SLICE )
    {
      for (int refIdx = 0; refIdx < getNumRefIdx(REF_PIC_LIST_1); refIdx++)
      {
        minPicDist = std::min(minPicDist, std::abs(currPOC - getRefPic(REF_PIC_LIST_1, refIdx)->getPOC()));
      }
    }
  }
  return (unsigned) minPicDist;
}

// ------------------------------------------------------------------------------------------------
// Video parameter set (VPS)
// ------------------------------------------------------------------------------------------------
VPS::VPS()
  : m_VPSId(0)
  , m_uiMaxLayers(1)
  , m_vpsMaxSubLayers(1)
  , m_vpsAllLayersSameNumSubLayersFlag (true)
  , m_vpsAllIndependentLayersFlag(true)
  , m_vpsEachLayerIsAnOlsFlag (1)
  , m_vpsOlsModeIdc (0)
  , m_vpsNumOutputLayerSets (1)
#if JVET_Q0786_PTL_only
  , m_vpsNumPtls (1)
#endif
  , m_vpsExtensionFlag()
#if JVET_P0118_HRD_ASPECTS
  , m_vpsGeneralHrdParamsPresentFlag(false)
  , m_vpsSublayerCpbParamsPresentFlag(false)
  , m_numOlsHrdParamsMinus1(0)
#endif
#if JVET_Q0814_DPB
  , m_totalNumOLSs( 0 )
  , m_numDpbParams( 0 )
  , m_sublayerDpbParamsPresentFlag( false )
  , m_targetOlsIdx( -1 )
#endif
{
  for (int i = 0; i < MAX_VPS_LAYERS; i++)
  {
    m_vpsLayerId[i] = 0;
    m_vpsIndependentLayerFlag[i] = true;
    for (int j = 0; j < MAX_VPS_LAYERS; j++)
    {
      m_vpsDirectRefLayerFlag[i][j] = 0;
      m_directRefLayerIdx[i][j] = MAX_VPS_LAYERS;
      m_interLayerRefIdx[i][i] = NOT_VALID;
    }
  }
  for (int i = 0; i < MAX_NUM_OLSS; i++)
  {
    for (int j = 0; j < MAX_VPS_LAYERS; j++)
    {
      m_vpsOlsOutputLayerFlag[i][j] = 0;
    }
#if JVET_Q0786_PTL_only
    if(i == 0)
      m_ptPresentFlag[i] = 1;
    else
      m_ptPresentFlag[i] = 0;
    m_ptlMaxTemporalId[i] = m_vpsMaxSubLayers - 1;
    m_olsPtlIdx[i] = 0;
#endif
#if JVET_P0118_HRD_ASPECTS
    m_hrdMaxTid[i] = m_vpsMaxSubLayers - 1;
    m_olsHrdIdx[i] = 0;
#endif
  }
}

VPS::~VPS()
{
}

#if JVET_Q0814_DPB
void VPS::deriveOutputLayerSets()
{
  if( m_uiMaxLayers == 1 )
  {
    m_totalNumOLSs = 1;
  }
  else if( m_vpsEachLayerIsAnOlsFlag || m_vpsOlsModeIdc < 2 )
  {
    m_totalNumOLSs = m_uiMaxLayers;
  }
  else if( m_vpsOlsModeIdc == 2 )
  {
    m_totalNumOLSs = m_vpsNumOutputLayerSets;
  }

  m_olsDpbParamsIdx.resize( m_totalNumOLSs );
  m_olsDpbPicSize.resize( m_totalNumOLSs, Size(0, 0) );
  m_numOutputLayersInOls.resize( m_totalNumOLSs );
  m_numLayersInOls.resize( m_totalNumOLSs );
  m_outputLayerIdInOls.resize( m_totalNumOLSs, std::vector<int>( m_uiMaxLayers, NOT_VALID ) );
  m_layerIdInOls.resize( m_totalNumOLSs, std::vector<int>( m_uiMaxLayers, NOT_VALID ) );

  std::vector<int> numRefLayers( m_uiMaxLayers );
  std::vector<std::vector<int>> outputLayerIdx( m_totalNumOLSs, std::vector<int>( m_uiMaxLayers, NOT_VALID ) );
  std::vector<std::vector<int>> layerIncludedInOlsFlag( m_totalNumOLSs, std::vector<int>( m_uiMaxLayers, 0 ) );
  std::vector<std::vector<int>> dependencyFlag( m_uiMaxLayers, std::vector<int>( m_uiMaxLayers, NOT_VALID ) );
  std::vector<std::vector<int>> refLayerIdx( m_uiMaxLayers, std::vector<int>( m_uiMaxLayers, NOT_VALID ) );
#if JVET_Q0118_CLEANUPS
  std::vector<int> layerUsedAsRefLayerFlag( m_uiMaxLayers, 0 );
  std::vector<int> layerUsedAsOutputLayerFlag( m_uiMaxLayers, NOT_VALID );
#endif

  for( int i = 0; i < m_uiMaxLayers; i++ )
  {
    int r = 0;

    for( int j = 0; j < m_uiMaxLayers; j++ )
    {
      dependencyFlag[i][j] = m_vpsDirectRefLayerFlag[i][j];

      for( int k = 0; k < i; k++ )
      {
        if( m_vpsDirectRefLayerFlag[i][k] && dependencyFlag[k][j] )
        {
          dependencyFlag[i][j] = 1;
        }
      }
#if JVET_Q0118_CLEANUPS
      if (m_vpsDirectRefLayerFlag[i][j])
      {
        layerUsedAsRefLayerFlag[j] = 1;
      }
#endif

      if( dependencyFlag[i][j] )
      {
        refLayerIdx[i][r++] = j;
      }
    }

    numRefLayers[i] = r;
  }

  m_numOutputLayersInOls[0] = 1;
  m_outputLayerIdInOls[0][0] = m_vpsLayerId[0];
#if JVET_Q0118_CLEANUPS
  layerUsedAsOutputLayerFlag[0] = 1;
  for (int i = 1; i < m_uiMaxLayers; i++)
  {
    if (m_vpsEachLayerIsAnOlsFlag || m_vpsOlsModeIdc < 2)
    {
      layerUsedAsOutputLayerFlag[i] = 1;
    }
    else
    {
      layerUsedAsOutputLayerFlag[i] = 0;
    }
  }
#endif

  for( int i = 1; i < m_totalNumOLSs; i++ )
  {
    if( m_vpsEachLayerIsAnOlsFlag || m_vpsOlsModeIdc == 0 )
    {
      m_numOutputLayersInOls[i] = 1;
      m_outputLayerIdInOls[i][0] = m_vpsLayerId[i];
    }
    else if( m_vpsOlsModeIdc == 1 )
    {
      m_numOutputLayersInOls[i] = i + 1;

      for( int j = 0; j < m_numOutputLayersInOls[i]; j++ )
      {
        m_outputLayerIdInOls[i][j] = m_vpsLayerId[j];
      }
    }
    else if( m_vpsOlsModeIdc == 2 )
    {
      int j = 0;
      for( int k = 0; k < m_uiMaxLayers; k++ )
      {
        if( m_vpsOlsOutputLayerFlag[i][k] )
        {
          layerIncludedInOlsFlag[i][k] = 1;
#if JVET_Q0118_CLEANUPS
          layerUsedAsOutputLayerFlag[k] = 1;
#endif
          outputLayerIdx[i][j] = k;
          m_outputLayerIdInOls[i][j++] = m_vpsLayerId[k];
        }
      }
      m_numOutputLayersInOls[i] = j;

      for( j = 0; j < m_numOutputLayersInOls[i]; j++ )
      {
        int idx = outputLayerIdx[i][j];
        for( int k = 0; k < numRefLayers[idx]; k++ )
        {
          layerIncludedInOlsFlag[i][refLayerIdx[idx][k]] = 1;
        }
      }
    }
  }
#if JVET_Q0118_CLEANUPS
  for (int i = 0; i < m_uiMaxLayers; i++)
  {
    CHECK(layerUsedAsRefLayerFlag[i] == 0 && layerUsedAsOutputLayerFlag[i] == 0, "There shall be no layer that is neither an output layer nor a direct reference layer");
  }
#endif

  m_numLayersInOls[0] = 1;
  m_layerIdInOls[0][0] = m_vpsLayerId[0];

  for( int i = 1; i < m_totalNumOLSs; i++ )
  {
    if( m_vpsEachLayerIsAnOlsFlag )
    {
      m_numLayersInOls[i] = 1;
      m_layerIdInOls[i][0] = m_vpsLayerId[i];
    }
    else if( m_vpsOlsModeIdc == 0 || m_vpsOlsModeIdc == 1 )
    {
      m_numLayersInOls[i] = i + 1;
      for( int j = 0; j < m_numLayersInOls[i]; j++ )
      {
        m_layerIdInOls[i][j] = m_vpsLayerId[j];
      }
    }
    else if( m_vpsOlsModeIdc == 2 )
    {
      int j = 0;
      for( int k = 0; k < m_uiMaxLayers; k++ )
      {
        if( layerIncludedInOlsFlag[i][k] )
        {
          m_layerIdInOls[i][j++] = m_vpsLayerId[k];
        }
      }

      m_numLayersInOls[i] = j;
    }
  }
}

void VPS::deriveTargetOutputLayerSet( int targetOlsIdx )
{
  m_targetOlsIdx = targetOlsIdx < 0 ? m_uiMaxLayers - 1 : targetOlsIdx;
  m_targetOutputLayerIdSet.clear();
  m_targetLayerIdSet.clear();

  for( int i = 0; i < m_numOutputLayersInOls[m_targetOlsIdx]; i++ )
  {
    m_targetOutputLayerIdSet.push_back( m_outputLayerIdInOls[m_targetOlsIdx][i] );
  }

  for( int i = 0; i < m_numLayersInOls[m_targetOlsIdx]; i++ )
  {
    m_targetLayerIdSet.push_back( m_layerIdInOls[m_targetOlsIdx][i] );
  }
}
#endif

// ------------------------------------------------------------------------------------------------
// Picture Header
// ------------------------------------------------------------------------------------------------

PicHeader::PicHeader()
: m_valid                                         ( 0 )
, m_nonReferencePictureFlag                       ( 0 )
, m_gdrPicFlag                                    ( 0 )
, m_noOutputOfPriorPicsFlag                       ( 0 )
, m_recoveryPocCnt                                ( 0 )
#if SPS_ID_CHECK
, m_noOutputBeforeRecoveryFlag                    ( false )
, m_handleCraAsCvsStartFlag                       ( false )
, m_handleGdrAsCvsStartFlag                       ( false )
#endif
, m_spsId                                         ( -1 )
, m_ppsId                                         ( -1 )
#if JVET_P0116_POC_MSB
, m_pocMsbPresentFlag                             ( 0 )
, m_pocMsbVal                                     ( 0 )
#endif
#if !JVET_Q0119_CLEANUPS
, m_subPicIdSignallingPresentFlag                 ( 0 )
, m_subPicIdLen                                   ( 0 )
#endif
#if JVET_Q0246_VIRTUAL_BOUNDARY_ENABLE_FLAG 
, m_virtualBoundariesEnabledFlag                  ( 0 )
, m_virtualBoundariesPresentFlag                  ( 0 )
#else
, m_loopFilterAcrossVirtualBoundariesDisabledFlag ( 0 )
#endif
, m_numVerVirtualBoundaries                       ( 0 )
, m_numHorVirtualBoundaries                       ( 0 )
#if !JVET_Q0155_COLOUR_ID
, m_colourPlaneId                                 ( 0 )
#endif
, m_picOutputFlag                                 ( true )
#if !JVET_Q0819_PH_CHANGES 
, m_picRplPresentFlag                             ( 0 )
#endif
, m_pRPL0                                         ( 0 )
, m_pRPL1                                         ( 0 )
, m_rpl0Idx                                       ( 0 )
, m_rpl1Idx                                       ( 0 )
, m_splitConsOverrideFlag                         ( 0 )
, m_cuQpDeltaSubdivIntra                          ( 0 )
, m_cuQpDeltaSubdivInter                          ( 0 )
, m_cuChromaQpOffsetSubdivIntra                   ( 0 )
, m_cuChromaQpOffsetSubdivInter                   ( 0 )
, m_enableTMVPFlag                                ( true )
#if JVET_Q0482_REMOVE_CONSTANT_PARAMS
, m_picColFromL0Flag                              ( true )
#endif
, m_mvdL1ZeroFlag                                 ( 0 )
#if !JVET_Q0798_SPS_NUMBER_MERGE_CANDIDATE
, m_maxNumMergeCand                               ( MRG_MAX_NUM_CANDS )
#endif
, m_maxNumAffineMergeCand                         ( AFFINE_MRG_MAX_NUM_CANDS )
, m_disFracMMVD                                   ( 0 )
, m_disBdofFlag                                   ( 0 )
, m_disDmvrFlag                                   ( 0 )
, m_disProfFlag                                   ( 0 )
#if !JVET_Q0798_SPS_NUMBER_MERGE_CANDIDATE 
#if !JVET_Q0806
, m_maxNumTriangleCand                            ( 0 )
#else
, m_maxNumGeoCand                                 ( 0 )
#endif
, m_maxNumIBCMergeCand                            ( IBC_MRG_MAX_NUM_CANDS )
#endif
, m_jointCbCrSignFlag                             ( 0 )
#if JVET_Q0819_PH_CHANGES 
, m_qpDelta                                       ( 0 )
#else
, m_saoEnabledPresentFlag                         ( 0 )
, m_alfEnabledPresentFlag                         ( 0 )
#endif
, m_numAlfAps                                     ( 0 )
, m_alfApsId                                      ( 0 )
, m_alfChromaApsId                                ( 0 )
, m_depQuantEnabledFlag                           ( 0 )
, m_signDataHidingEnabledFlag                     ( 0 )
#if !JVET_Q0819_PH_CHANGES  
, m_deblockingFilterOverridePresentFlag           ( 0 )
#endif
, m_deblockingFilterOverrideFlag                  ( 0 )
, m_deblockingFilterDisable                       ( 0 )
, m_deblockingFilterBetaOffsetDiv2                ( 0 )
, m_deblockingFilterTcOffsetDiv2                  ( 0 )
#if JVET_Q0121_DEBLOCKING_CONTROL_PARAMETERS
, m_deblockingFilterCbBetaOffsetDiv2              ( 0 )
, m_deblockingFilterCbTcOffsetDiv2                ( 0 )
, m_deblockingFilterCrBetaOffsetDiv2              ( 0 )
, m_deblockingFilterCrTcOffsetDiv2                ( 0 )
#endif
, m_lmcsEnabledFlag                               ( 0 )
, m_lmcsApsId                                     ( -1 )
, m_lmcsAps                                       ( nullptr )
, m_lmcsChromaResidualScaleFlag                   ( 0 )
#if JVET_Q0346_SCALING_LIST_USED_IN_SH
, m_explicitScalingListEnabledFlag                ( 0 )
#else
, m_scalingListPresentFlag                        ( 0 )
#endif
, m_scalingListApsId                              ( -1 )
, m_scalingListAps                                ( nullptr )
#if JVET_Q0819_PH_CHANGES 
, m_numL0Weights                                  ( 0 )
, m_numL1Weights                                  ( 0 )
#endif
{
#if !JVET_Q0119_CLEANUPS
  memset(m_subPicId,                                0,    sizeof(m_subPicId));
#endif
  memset(m_virtualBoundariesPosX,                   0,    sizeof(m_virtualBoundariesPosX));
  memset(m_virtualBoundariesPosY,                   0,    sizeof(m_virtualBoundariesPosY));
  memset(m_saoEnabledFlag,                          0,    sizeof(m_saoEnabledFlag));
  memset(m_alfEnabledFlag,                          0,    sizeof(m_alfEnabledFlag));
  memset(m_minQT,                                   0,    sizeof(m_minQT));
  memset(m_maxMTTHierarchyDepth,                    0,    sizeof(m_maxMTTHierarchyDepth));
  memset(m_maxBTSize,                               0,    sizeof(m_maxBTSize));
  memset(m_maxTTSize,                               0,    sizeof(m_maxTTSize));

  m_localRPL0.setNumberOfActivePictures(0);
  m_localRPL0.setNumberOfShorttermPictures(0);
  m_localRPL0.setNumberOfLongtermPictures(0);
  m_localRPL0.setLtrpInSliceHeaderFlag(0);
  m_localRPL0.setNumberOfInterLayerPictures( 0 );

  m_localRPL1.setNumberOfActivePictures(0);
  m_localRPL1.setNumberOfShorttermPictures(0);
  m_localRPL1.setNumberOfLongtermPictures(0);
  m_localRPL1.setLtrpInSliceHeaderFlag(0);
  m_localRPL1.setNumberOfInterLayerPictures( 0 );

  m_alfApsId.resize(0);

#if JVET_Q0819_PH_CHANGES 
  resetWpScaling();
#endif
}

PicHeader::~PicHeader()
{
  m_alfApsId.resize(0);
}

/**
 - initialize picture header to defaut state
 */
void PicHeader::initPicHeader()
{
  m_valid                                         = 0;
  m_nonReferencePictureFlag                       = 0;
  m_gdrPicFlag                                    = 0;
  m_noOutputOfPriorPicsFlag                       = 0;
  m_recoveryPocCnt                                = 0;
  m_spsId                                         = -1;
  m_ppsId                                         = -1;
#if JVET_P0116_POC_MSB
  m_pocMsbPresentFlag                             = 0;
  m_pocMsbVal                                     = 0;
#endif
#if !JVET_Q0119_CLEANUPS
  m_subPicIdSignallingPresentFlag                 = 0;
  m_subPicIdLen                                   = 0;
#endif
#if JVET_Q0246_VIRTUAL_BOUNDARY_ENABLE_FLAG 
  m_virtualBoundariesEnabledFlag                  = 0;
  m_virtualBoundariesPresentFlag                  = 0;
#else
  m_loopFilterAcrossVirtualBoundariesDisabledFlag = 0;
#endif
  m_numVerVirtualBoundaries                       = 0;
  m_numHorVirtualBoundaries                       = 0;
#if !JVET_Q0155_COLOUR_ID
  m_colourPlaneId                                 = 0;
#endif
  m_picOutputFlag                                 = true;
#if !JVET_Q0819_PH_CHANGES 
  m_picRplPresentFlag                             = 0;
#endif
  m_pRPL0                                         = 0;
  m_pRPL1                                         = 0;
  m_rpl0Idx                                       = 0;
  m_rpl1Idx                                       = 0;
  m_splitConsOverrideFlag                         = 0;
  m_cuQpDeltaSubdivIntra                          = 0;
  m_cuQpDeltaSubdivInter                          = 0;
  m_cuChromaQpOffsetSubdivIntra                   = 0;
  m_cuChromaQpOffsetSubdivInter                   = 0;
  m_enableTMVPFlag                                = true;
#if JVET_Q0482_REMOVE_CONSTANT_PARAMS
  m_picColFromL0Flag                              = true;
#endif
  m_mvdL1ZeroFlag                                 = 0;
#if !JVET_Q0798_SPS_NUMBER_MERGE_CANDIDATE 
  m_maxNumMergeCand                               = MRG_MAX_NUM_CANDS;
#endif
  m_maxNumAffineMergeCand                         = AFFINE_MRG_MAX_NUM_CANDS;
  m_disFracMMVD                                   = 0;
  m_disBdofFlag                                   = 0;
  m_disDmvrFlag                                   = 0;
  m_disProfFlag                                   = 0;
#if !JVET_Q0798_SPS_NUMBER_MERGE_CANDIDATE 
#if !JVET_Q0806
  m_maxNumTriangleCand                            = 0;
#else
  m_maxNumGeoCand                                 = 0;
#endif
  m_maxNumIBCMergeCand                            = IBC_MRG_MAX_NUM_CANDS;
#endif
  m_jointCbCrSignFlag                             = 0;
#if JVET_Q0819_PH_CHANGES 
  m_qpDelta                                       = 0;
#else
  m_saoEnabledPresentFlag                         = 0;
  m_alfEnabledPresentFlag                         = 0;
#endif
  m_numAlfAps                                     = 0;
  m_alfChromaApsId                                = 0;
  m_depQuantEnabledFlag                           = 0;
  m_signDataHidingEnabledFlag                     = 0;
#if !JVET_Q0819_PH_CHANGES 
  m_deblockingFilterOverridePresentFlag           = 0;
#endif
  m_deblockingFilterOverrideFlag                  = 0;
  m_deblockingFilterDisable                       = 0;
  m_deblockingFilterBetaOffsetDiv2                = 0;
  m_deblockingFilterTcOffsetDiv2                  = 0;
#if JVET_Q0121_DEBLOCKING_CONTROL_PARAMETERS
  m_deblockingFilterCbBetaOffsetDiv2              = 0;
  m_deblockingFilterCbTcOffsetDiv2                = 0;
  m_deblockingFilterCrBetaOffsetDiv2              = 0;
  m_deblockingFilterCrTcOffsetDiv2                = 0;
#endif
  m_lmcsEnabledFlag                               = 0;
  m_lmcsApsId                                     = -1;
  m_lmcsAps                                       = nullptr;
  m_lmcsChromaResidualScaleFlag                   = 0;
#if JVET_Q0346_SCALING_LIST_USED_IN_SH
  m_explicitScalingListEnabledFlag                = 0;
#else
  m_scalingListPresentFlag                        = 0;
#endif
  m_scalingListApsId                              = -1;
  m_scalingListAps                                = nullptr;
#if JVET_Q0819_PH_CHANGES 
  m_numL0Weights                                  = 0;
  m_numL1Weights                                  = 0;
#endif
#if !JVET_Q0119_CLEANUPS
  memset(m_subPicId,                                0,    sizeof(m_subPicId));
#endif
  memset(m_virtualBoundariesPosX,                   0,    sizeof(m_virtualBoundariesPosX));
  memset(m_virtualBoundariesPosY,                   0,    sizeof(m_virtualBoundariesPosY));
  memset(m_saoEnabledFlag,                          0,    sizeof(m_saoEnabledFlag));
  memset(m_alfEnabledFlag,                          0,    sizeof(m_alfEnabledFlag));
  memset(m_minQT,                                   0,    sizeof(m_minQT));
  memset(m_maxMTTHierarchyDepth,                    0,    sizeof(m_maxMTTHierarchyDepth));
  memset(m_maxBTSize,                               0,    sizeof(m_maxBTSize));
  memset(m_maxTTSize,                               0,    sizeof(m_maxTTSize));

  m_localRPL0.setNumberOfActivePictures(0);
  m_localRPL0.setNumberOfShorttermPictures(0);
  m_localRPL0.setNumberOfLongtermPictures(0);
  m_localRPL0.setLtrpInSliceHeaderFlag(0);

  m_localRPL1.setNumberOfActivePictures(0);
  m_localRPL1.setNumberOfShorttermPictures(0);
  m_localRPL1.setNumberOfLongtermPictures(0);
  m_localRPL1.setLtrpInSliceHeaderFlag(0);

  m_alfApsId.resize(0);
}

#if JVET_Q0819_PH_CHANGES 
void PicHeader::getWpScaling(RefPicList e, int iRefIdx, WPScalingParam *&wp) const
{
  CHECK(e >= NUM_REF_PIC_LIST_01, "Invalid picture reference list");
  wp = (WPScalingParam *) m_weightPredTable[e][iRefIdx];
}

void PicHeader::resetWpScaling()
{
  for ( int e=0 ; e<NUM_REF_PIC_LIST_01 ; e++ )
  {
    for ( int i=0 ; i<MAX_NUM_REF ; i++ )
    {
      for ( int yuv=0 ; yuv<MAX_NUM_COMPONENT ; yuv++ )
      {
        WPScalingParam  *pwp = &(m_weightPredTable[e][i][yuv]);
        pwp->bPresentFlag      = false;
        pwp->uiLog2WeightDenom = 0;
        pwp->iWeight           = 1;
        pwp->iOffset           = 0;
      }
    }
  }
}
#endif

// ------------------------------------------------------------------------------------------------
// Sequence parameter set (SPS)
// ------------------------------------------------------------------------------------------------
SPSRExt::SPSRExt()
 : m_transformSkipRotationEnabledFlag   (false)
 , m_transformSkipContextEnabledFlag    (false)
// m_rdpcmEnabledFlag initialized below
 , m_extendedPrecisionProcessingFlag    (false)
 , m_intraSmoothingDisabledFlag         (false)
 , m_highPrecisionOffsetsEnabledFlag    (false)
 , m_persistentRiceAdaptationEnabledFlag(false)
 , m_cabacBypassAlignmentEnabledFlag    (false)
{
  for (uint32_t signallingModeIndex = 0; signallingModeIndex < NUMBER_OF_RDPCM_SIGNALLING_MODES; signallingModeIndex++)
  {
    m_rdpcmEnabledFlag[signallingModeIndex] = false;
  }
}


SPS::SPS()
: m_SPSId                     (  0)
#if !JVET_Q0117_PARAMETER_SETS_CLEANUP
, m_decodingParameterSetId    (  0 )
#endif
, m_VPSId                     ( 0 )
, m_affineAmvrEnabledFlag     ( false )
, m_DMVR                      ( false )
, m_MMVD                      ( false )
, m_SBT                       ( false )
, m_ISP                       ( false )
, m_chromaFormatIdc           (CHROMA_420)
, m_separateColourPlaneFlag   ( 0 )
, m_uiMaxTLayers              (  1)
#if JVET_P0117_PTL_SCALABILITY
, m_ptlDpbHrdParamsPresentFlag (1)
, m_SubLayerDpbParamsFlag      (0)
#endif
// Structure
, m_maxWidthInLumaSamples     (352)
, m_maxHeightInLumaSamples    (288)
#if JVET_Q0119_CLEANUPS
, m_subPicInfoPresentFlag     (false)
#else
, m_subPicPresentFlag         (0)
#endif
, m_numSubPics(1)
#if JVET_Q0119_CLEANUPS
, m_subPicIdMappingExplicitlySignalledFlag ( false )
, m_subPicIdMappingInSpsFlag ( false )
#else
, m_subPicIdPresentFlag(0)
, m_subPicIdSignallingPresentFlag(0)
#endif
, m_subPicIdLen(16)
#if JVET_Q0468_Q0469_MIN_LUMA_CB_AND_MIN_QT_FIX
, m_log2MinCodingBlockSize    (  2)
#else
, m_log2MinCodingBlockSize    (  0)
, m_log2DiffMaxMinCodingBlockSize(0)
#endif
, m_CTUSize(0)
, m_minQT{ 0, 0, 0 }
, m_maxMTTHierarchyDepth{ MAX_BT_DEPTH, MAX_BT_DEPTH_INTER, MAX_BT_DEPTH_C }
#if JVET_Q0330_BLOCK_PARTITION
, m_maxBTSize{ 0, 0, 0 }
, m_maxTTSize{ 0, 0, 0 }
#else
, m_maxBTSize{ MAX_BT_SIZE,  MAX_BT_SIZE_INTER,  MAX_BT_SIZE_C }
, m_maxTTSize{ MAX_TT_SIZE,  MAX_TT_SIZE_INTER,  MAX_TT_SIZE_C }
#endif
, m_uiMaxCUWidth              ( 32)
, m_uiMaxCUHeight             ( 32)
#if !JVET_Q0468_Q0469_MIN_LUMA_CB_AND_MIN_QT_FIX
, m_uiMaxCodingDepth          (  3)
#endif
, m_numRPL0                   ( 0 )
, m_numRPL1                   ( 0 )
, m_rpl1CopyFromRpl0Flag      ( false )
, m_rpl1IdxPresentFlag        ( false )
, m_allRplEntriesHasSameSignFlag ( true )
, m_bLongTermRefsPresent      (false)
// Tool list
, m_transformSkipEnabledFlag  (false)
#if JVET_Q0183_SPS_TRANSFORM_SKIP_MODE_CONTROL
, m_log2MaxTransformSkipBlockSize (2)
#endif
#if JVET_Q0089_SLICE_LOSSLESS_CODING_CHROMA_BDPCM
, m_BDPCMEnabledFlag          (false)
#else
, m_BDPCMEnabled              (0)
#endif
, m_JointCbCrEnabledFlag      (false)
#if JVET_Q0151_Q0205_ENTRYPOINTS
, m_entropyCodingSyncEnabledFlag(false)
, m_entropyCodingSyncEntryPointPresentFlag(false)
#endif
, m_sbtmvpEnabledFlag         (false)
, m_bdofEnabledFlag           (false)
, m_fpelMmvdEnabledFlag       ( false )
, m_BdofControlPresentFlag    ( false )
, m_DmvrControlPresentFlag    ( false )
, m_ProfControlPresentFlag    ( false )
, m_uiBitsForPOC              (  8)
#if JVET_P0116_POC_MSB
, m_pocMsbFlag                ( false )
, m_pocMsbLen                 ( 1 )
#endif
#if JVET_Q0400_EXTRA_BITS
, m_numExtraPHBitsBytes       ( 0 )
, m_numExtraSHBitsBytes       ( 0 )
#endif
, m_numLongTermRefPicSPS      (  0)

, m_log2MaxTbSize             (  6)

, m_useWeightPred             (false)
, m_useWeightedBiPred         (false)
, m_saoEnabledFlag            (false)
, m_bTemporalIdNestingFlag    (false)
, m_scalingListEnabledFlag    (false)
#if JVET_Q0246_VIRTUAL_BOUNDARY_ENABLE_FLAG 
, m_virtualBoundariesEnabledFlag( 0 )
, m_virtualBoundariesPresentFlag( 0 )
#else
, m_loopFilterAcrossVirtualBoundariesDisabledFlag(0)
#endif
, m_numVerVirtualBoundaries(0)
, m_numHorVirtualBoundaries(0)
#if JVET_P0118_HRD_ASPECTS
, m_generalHrdParametersPresentFlag(false)
#else
, m_hrdParametersPresentFlag  (false)
#endif
#if JVET_Q0042_VUI
, m_fieldSeqFlag              (false)
#endif
, m_vuiParametersPresentFlag  (false)
, m_vuiParameters             ()
, m_wrapAroundEnabledFlag     (false)
, m_wrapAroundOffset          (  0)
, m_IBCFlag                   (  0)
, m_PLTMode                   (  0)
, m_lmcsEnabled               (false)
, m_AMVREnabledFlag                       ( false )
, m_LMChroma                  ( false )
, m_horCollocatedChromaFlag   ( true )
, m_verCollocatedChromaFlag   ( false )
, m_IntraMTS                  ( false )
, m_InterMTS                  ( false )
, m_LFNST                     ( false )
, m_Affine                    ( false )
, m_AffineType                ( false )
, m_PROF                      ( false )
, m_ciip                   ( false )
#if !JVET_Q0806
, m_Triangle                  ( false )
#else
, m_Geo                       ( false )
#endif
#if LUMA_ADAPTIVE_DEBLOCKING_FILTER_QP_OFFSET
, m_LadfEnabled               ( false )
, m_LadfNumIntervals          ( 0 )
, m_LadfQpOffset              { 0 }
, m_LadfIntervalLowerBound    { 0 }
#endif
, m_MRL                       ( false )
, m_MIP                       ( false )
, m_GDREnabledFlag            ( true )
, m_SubLayerCbpParametersPresentFlag ( true )
, m_rprEnabledFlag            ( false )
#if JVET_Q0798_SPS_NUMBER_MERGE_CANDIDATE
, m_maxNumMergeCand(MRG_MAX_NUM_CANDS)
, m_maxNumAffineMergeCand(AFFINE_MRG_MAX_NUM_CANDS)
, m_maxNumIBCMergeCand(IBC_MRG_MAX_NUM_CANDS)
, m_maxNumGeoCand(0)
#endif
{
  for(int ch=0; ch<MAX_NUM_CHANNEL_TYPE; ch++)
  {
    m_bitDepths.recon[ch] = 8;
    m_qpBDOffset   [ch] = 0;
  }

  for ( int i = 0; i < MAX_TLAYER; i++ )
  {
    m_uiMaxLatencyIncreasePlus1[i] = 0;
    m_uiMaxDecPicBuffering[i] = 1;
    m_numReorderPics[i]       = 0;
  }

  ::memset(m_ltRefPicPocLsbSps, 0, sizeof(m_ltRefPicPocLsbSps));
  ::memset(m_usedByCurrPicLtSPSFlag, 0, sizeof(m_usedByCurrPicLtSPSFlag));
  ::memset(m_virtualBoundariesPosX, 0, sizeof(m_virtualBoundariesPosX));
  ::memset(m_virtualBoundariesPosY, 0, sizeof(m_virtualBoundariesPosY));
#if JVET_Q0179_SCALING_WINDOW_SIZE_CONSTRAINT
  ::memset(m_ppsValidFlag, 0, sizeof(m_ppsValidFlag));
#endif
}

SPS::~SPS()
{
}

void  SPS::createRPLList0(int numRPL)
{
  m_RPLList0.destroy();
  m_RPLList0.create(numRPL);
  m_numRPL0 = numRPL;
  m_rpl1IdxPresentFlag = (m_numRPL0 != m_numRPL1) ? true : false;
}
void  SPS::createRPLList1(int numRPL)
{
  m_RPLList1.destroy();
  m_RPLList1.create(numRPL);
  m_numRPL1 = numRPL;

  m_rpl1IdxPresentFlag = (m_numRPL0 != m_numRPL1) ? true : false;
}


const int SPS::m_winUnitX[]={1,2,2,1};
const int SPS::m_winUnitY[]={1,2,1,1};

void ChromaQpMappingTable::setParams(const ChromaQpMappingTableParams &params, const int qpBdOffset)
{
  m_qpBdOffset = qpBdOffset;
  m_sameCQPTableForAllChromaFlag = params.m_sameCQPTableForAllChromaFlag;
  m_numQpTables = params.m_numQpTables;

  for (int i = 0; i < MAX_NUM_CQP_MAPPING_TABLES; i++)
  {
    m_numPtsInCQPTableMinus1[i] = params.m_numPtsInCQPTableMinus1[i];
    m_deltaQpInValMinus1[i] = params.m_deltaQpInValMinus1[i];
    m_qpTableStartMinus26[i] = params.m_qpTableStartMinus26[i];
    m_deltaQpOutVal[i] = params.m_deltaQpOutVal[i];
  }
}
void ChromaQpMappingTable::derivedChromaQPMappingTables()
{
  for (int i = 0; i < getNumQpTables(); i++)
  {
    const int qpBdOffsetC = m_qpBdOffset;
    const int numPtsInCQPTableMinus1 = getNumPtsInCQPTableMinus1(i);
    std::vector<int> qpInVal(numPtsInCQPTableMinus1 + 2), qpOutVal(numPtsInCQPTableMinus1 + 2);

    qpInVal[0] = getQpTableStartMinus26(i) + 26;
    qpOutVal[0] = qpInVal[0];
    for (int j = 0; j <= getNumPtsInCQPTableMinus1(i); j++)
    {
      qpInVal[j + 1] = qpInVal[j] + getDeltaQpInValMinus1(i, j) + 1;
      qpOutVal[j + 1] = qpOutVal[j] + getDeltaQpOutVal(i, j);
    }

    for (int j = 0; j <= getNumPtsInCQPTableMinus1(i); j++)
    {
      CHECK(qpInVal[j]  < -qpBdOffsetC || qpInVal[j]  > MAX_QP, "qpInVal out of range");
      CHECK(qpOutVal[j] < -qpBdOffsetC || qpOutVal[j] > MAX_QP, "qpOutVal out of range");
    }

    m_chromaQpMappingTables[i][qpInVal[0]] = qpOutVal[0];
    for (int k = qpInVal[0] - 1; k >= -qpBdOffsetC; k--)
    {
      m_chromaQpMappingTables[i][k] = Clip3(-qpBdOffsetC, MAX_QP, m_chromaQpMappingTables[i][k + 1] - 1);
    }
    for (int j = 0; j <= numPtsInCQPTableMinus1; j++)
    {
      int sh = (getDeltaQpInValMinus1(i, j) + 1) >> 1;
      for (int k = qpInVal[j] + 1, m = 1; k <= qpInVal[j + 1]; k++, m++)
      {
        m_chromaQpMappingTables[i][k] = m_chromaQpMappingTables[i][qpInVal[j]]
          + ((qpOutVal[j + 1] - qpOutVal[j]) * m + sh) / (getDeltaQpInValMinus1(i, j) + 1);
      }
    }
    for (int k = qpInVal[numPtsInCQPTableMinus1 + 1] + 1; k <= MAX_QP; k++)
    {
      m_chromaQpMappingTables[i][k] = Clip3(-qpBdOffsetC, MAX_QP, m_chromaQpMappingTables[i][k - 1] + 1);
    }
  }
}

SliceMap::SliceMap()
: m_sliceID              (0)
, m_numTilesInSlice      (0)
, m_numCtuInSlice        (0)
{
  m_ctuAddrInSlice.clear();
}

SliceMap::~SliceMap()
{
  m_numCtuInSlice = 0;
  m_ctuAddrInSlice.clear();
}

RectSlice::RectSlice()
: m_tileIdx            (0)
, m_sliceWidthInTiles  (0)
, m_sliceHeightInTiles (0)
, m_numSlicesInTile    (0)
, m_sliceHeightInCtu   (0)
{
}

RectSlice::~RectSlice()
{
}

#if JVET_O1143_SUBPIC_BOUNDARY
SubPic::SubPic()
: m_subPicID              (0)
, m_numCTUsInSubPic       (0)
, m_subPicCtuTopLeftX     (0)
, m_subPicCtuTopLeftY     (0)
, m_subPicWidth           (0)
, m_subPicHeight          (0)
, m_firstCtuInSubPic      (0)
, m_lastCtuInSubPic       (0)
, m_subPicLeft            (0)
, m_subPicRight           (0)
, m_subPicTop             (0)
, m_subPicBottom          (0)
, m_treatedAsPicFlag                  (false)
, m_loopFilterAcrossSubPicEnabledFlag (false)
{
  m_ctuAddrInSubPic.clear();
}

SubPic::~SubPic()
{
  m_ctuAddrInSubPic.clear();
}
#endif

#if !REMOVE_PPS_REXT
PPSRExt::PPSRExt()
: m_crossComponentPredictionEnabledFlag(false)
#if JVET_Q0441_SAO_MOD_12_BIT
{
#else
// m_log2SaoOffsetScale initialized below
{
  for(int ch=0; ch<MAX_NUM_CHANNEL_TYPE; ch++)
  {
    m_log2SaoOffsetScale[ch] = 0;
  }
#endif
}
#endif
  
PPS::PPS()
: m_PPSId                            (0)
, m_SPSId                            (0)
, m_picInitQPMinus26                 (0)
, m_useDQP                           (false)
, m_bSliceChromaQpFlag               (false)
, m_chromaCbQpOffset                 (0)
, m_chromaCrQpOffset                 (0)
, m_chromaCbCrQpOffset               (0)
, m_chromaQpOffsetListLen              (0)
, m_numRefIdxL0DefaultActive         (1)
, m_numRefIdxL1DefaultActive         (1)
, m_rpl1IdxPresentFlag               (false)
, m_numSubPics                       (1)
#if JVET_Q0119_CLEANUPS
, m_subPicIdMappingInPpsFlag         (0)
#else
, m_subPicIdSignallingPresentFlag    (0)
#endif
, m_subPicIdLen                      (16)
, m_noPicPartitionFlag               (1)
, m_log2CtuSize                      (0)
, m_ctuSize                          (0)
, m_picWidthInCtu                    (0)
, m_picHeightInCtu                   (0)
, m_numTileCols                      (1)
, m_numTileRows                      (1)
, m_rectSliceFlag                    (1)  
, m_singleSlicePerSubPicFlag         (0)
, m_numSlicesInPic                   (1)
, m_tileIdxDeltaPresentFlag          (0)
, m_loopFilterAcrossTilesEnabledFlag (1)
, m_loopFilterAcrossSlicesEnabledFlag(0)
#if !JVET_Q0183_SPS_TRANSFORM_SKIP_MODE_CONTROL
  , m_log2MaxTransformSkipBlockSize    (2)
#endif
#if !JVET_Q0151_Q0205_ENTRYPOINTS
, m_entropyCodingSyncEnabledFlag     (false)
#endif
#if !JVET_Q0482_REMOVE_CONSTANT_PARAMS
, m_constantSliceHeaderParamsEnabledFlag (false)
, m_PPSDepQuantEnabledIdc            (0)
, m_PPSRefPicListSPSIdc0             (0)
, m_PPSRefPicListSPSIdc1             (0)
, m_PPSMvdL1ZeroIdc                  (0)
, m_PPSCollocatedFromL0Idc           (0)
, m_PPSSixMinusMaxNumMergeCandPlus1  (0)
#if !JVET_Q0806
, m_PPSMaxNumMergeCandMinusMaxNumTriangleCandPlus1 (0)
#else
, m_PPSMaxNumMergeCandMinusMaxNumGeoCandPlus1 (0)
#endif
#endif
, m_cabacInitPresentFlag             (false)
, m_pictureHeaderExtensionPresentFlag(0)
, m_sliceHeaderExtensionPresentFlag  (false)
, m_listsModificationPresentFlag     (0)
#if JVET_Q0819_PH_CHANGES
, m_rplInfoInPhFlag                  (0)
, m_dbfInfoInPhFlag                  (0)
, m_saoInfoInPhFlag                  (0)
, m_alfInfoInPhFlag                  (0)
, m_wpInfoInPhFlag                   (0)
, m_qpDeltaInfoInPhFlag              (0)
#endif
#if SPS_ID_CHECK
, m_mixedNaluTypesInPicFlag          ( false )
#endif
, m_picWidthInLumaSamples(352)
, m_picHeightInLumaSamples( 288 )
#if !REMOVE_PPS_REXT
, m_ppsRangeExtension                ()
#endif
, pcv                                (NULL)
{
  m_ChromaQpAdjTableIncludingNullEntry[0].u.comp.CbOffset = 0; // Array includes entry [0] for the null offset used when cu_chroma_qp_offset_flag=0. This is initialised here and never subsequently changed.
  m_ChromaQpAdjTableIncludingNullEntry[0].u.comp.CrOffset = 0;
  m_ChromaQpAdjTableIncludingNullEntry[0].u.comp.JointCbCrOffset = 0;
  m_tileColWidth.clear();
  m_tileRowHeight.clear();
  m_tileColBd.clear();
  m_tileRowBd.clear();
  m_ctuToTileCol.clear();
  m_ctuToTileRow.clear();
  m_ctuToSubPicIdx.clear();
  m_rectSlices.clear();
  m_sliceMap.clear();
#if JVET_O1143_SUBPIC_BOUNDARY
  m_subPics.clear();
#endif
}

PPS::~PPS()
{
  m_tileColWidth.clear();
  m_tileRowHeight.clear();
  m_tileColBd.clear();
  m_tileRowBd.clear();
  m_ctuToTileCol.clear();
  m_ctuToTileRow.clear();
  m_ctuToSubPicIdx.clear();
  m_rectSlices.clear();
  m_sliceMap.clear();

#if JVET_O1143_SUBPIC_BOUNDARY
  m_subPics.clear();
#endif
  delete pcv;
}

/**
 - reset tile and slice parameters and lists
 */
void PPS::resetTileSliceInfo()
{
  m_numExpTileCols = 0;
  m_numExpTileRows = 0;
  m_numTileCols    = 0;
  m_numTileRows    = 0;
  m_numSlicesInPic = 0;
  m_tileColWidth.clear();
  m_tileRowHeight.clear();
  m_tileColBd.clear();
  m_tileRowBd.clear();
  m_ctuToTileCol.clear();
  m_ctuToTileRow.clear();
  m_ctuToSubPicIdx.clear();
  m_rectSlices.clear();
  m_sliceMap.clear();
}

/**
 - initialize tile row/column sizes and boundaries
 */
void PPS::initTiles()
{
  int       colIdx, rowIdx;
  int       ctuX, ctuY;
  
  // check explicit tile column sizes
  uint32_t  remainingWidthInCtu  = m_picWidthInCtu;
  for( colIdx = 0; colIdx < m_numExpTileCols; colIdx++ )
  {
    CHECK(m_tileColWidth[colIdx] > remainingWidthInCtu,    "Tile column width exceeds picture width");
    remainingWidthInCtu -= m_tileColWidth[colIdx];
  }

  // divide remaining picture width into uniform tile columns
  uint32_t  uniformTileColWidth = m_tileColWidth[colIdx-1];
  while( remainingWidthInCtu > 0 ) 
  {
    CHECK(colIdx >= MAX_TILE_COLS, "Number of tile columns exceeds valid range");
    uniformTileColWidth = std::min(remainingWidthInCtu, uniformTileColWidth);
    m_tileColWidth.push_back( uniformTileColWidth );
    remainingWidthInCtu -= uniformTileColWidth;
    colIdx++;
  }
  m_numTileCols = colIdx;
    
  // check explicit tile row sizes
  uint32_t  remainingHeightInCtu  = m_picHeightInCtu;
  for( rowIdx = 0; rowIdx < m_numExpTileRows; rowIdx++ )
  {
    CHECK(m_tileRowHeight[rowIdx] > remainingHeightInCtu,     "Tile row height exceeds picture height");
    remainingHeightInCtu -= m_tileRowHeight[rowIdx];
  }
    
  // divide remaining picture height into uniform tile rows
  uint32_t  uniformTileRowHeight = m_tileRowHeight[rowIdx - 1];
  while( remainingHeightInCtu > 0 ) 
  {
    CHECK(rowIdx >= MAX_TILE_ROWS, "Number of tile rows exceeds valid range");
    uniformTileRowHeight = std::min(remainingHeightInCtu, uniformTileRowHeight);
    m_tileRowHeight.push_back( uniformTileRowHeight );
    remainingHeightInCtu -= uniformTileRowHeight;
    rowIdx++;
  }
  m_numTileRows = rowIdx;

  // set left column bounaries
  m_tileColBd.push_back( 0 );
  for( colIdx = 0; colIdx < m_numTileCols; colIdx++ )
  {
    m_tileColBd.push_back( m_tileColBd[ colIdx ] + m_tileColWidth[ colIdx ] );
  }
  
  // set top row bounaries
  m_tileRowBd.push_back( 0 );
  for( rowIdx = 0; rowIdx < m_numTileRows; rowIdx++ )
  {
    m_tileRowBd.push_back( m_tileRowBd[ rowIdx ] + m_tileRowHeight[ rowIdx ] );
  }

  // set mapping between horizontal CTU address and tile column index
  colIdx = 0;
  for( ctuX = 0; ctuX <= m_picWidthInCtu; ctuX++ ) 
  {
    if( ctuX == m_tileColBd[ colIdx + 1 ] )
    {
      colIdx++;
    }
    m_ctuToTileCol.push_back( colIdx );
  }
  
  // set mapping between vertical CTU address and tile row index
  rowIdx = 0;
  for( ctuY = 0; ctuY <= m_picHeightInCtu; ctuY++ ) 
  {
    if( ctuY == m_tileRowBd[ rowIdx + 1 ] )
    {
      rowIdx++;
    }
    m_ctuToTileRow.push_back( rowIdx );
  }
}

/**
 - initialize memory for rectangular slice parameters
 */
void PPS::initRectSlices()
{ 
  CHECK(m_numSlicesInPic > MAX_SLICES, "Number of slices in picture exceeds valid range");
  m_rectSlices.resize(m_numSlicesInPic);
}

/**
 - initialize mapping between rectangular slices and CTUs
 */
void PPS::initRectSliceMap(const SPS  *sps)
{
  uint32_t  ctuY;
  uint32_t  tileX, tileY;

  if (sps)
  {
    m_ctuToSubPicIdx.resize(getPicWidthInCtu() * getPicHeightInCtu());
    if (sps->getNumSubPics() > 1)
    {
      for (int i = 0; i <= sps->getNumSubPics() - 1; i++)
      {
        for (int y = sps->getSubPicCtuTopLeftY(i); y < sps->getSubPicCtuTopLeftY(i) + sps->getSubPicHeight(i); y++)
        {
          for (int x = sps->getSubPicCtuTopLeftX(i); x < sps->getSubPicCtuTopLeftX(i) + sps->getSubPicWidth(i); x++)
          {
            m_ctuToSubPicIdx[ x+ y * getPicWidthInCtu()] = i;
          }
        }
      }
    }
    else
    {
      for (int i = 0; i < getPicWidthInCtu() * getPicHeightInCtu(); i++)
      {
        m_ctuToSubPicIdx[i] = 0;
      }
    }
  }

#if JVET_Q0817
  if( getSingleSlicePerSubPicFlag() )
#else
  if ((getNumSubPics() > 0) && getSingleSlicePerSubPicFlag())
#endif
  {
    CHECK (sps==nullptr, "RectSliceMap can only be initialized for slice_per_sub_pic_flag with a valid SPS");
    m_numSlicesInPic = sps->getNumSubPics();

    // allocate new memory for slice list
    CHECK(m_numSlicesInPic > MAX_SLICES, "Number of slices in picture exceeds valid range");
    m_sliceMap.resize( m_numSlicesInPic );

    // Q2001 v15 equation 29
    std::vector<uint32_t> subpicWidthInTiles;
    std::vector<uint32_t> subpicHeightInTiles;
    std::vector<uint32_t> subpicHeightLessThanOneTileFlag;
    subpicWidthInTiles.resize(sps->getNumSubPics());
    subpicHeightInTiles.resize(sps->getNumSubPics());
    subpicHeightLessThanOneTileFlag.resize(sps->getNumSubPics());
    for (uint32_t i = 0; i <sps->getNumSubPics(); i++)
    {
      uint32_t leftX = sps->getSubPicCtuTopLeftX(i);
      uint32_t rightX = leftX + sps->getSubPicWidth(i) - 1;
      subpicWidthInTiles[i] = m_ctuToTileCol[rightX] + 1 - m_ctuToTileCol[leftX];
      
      uint32_t topY = sps->getSubPicCtuTopLeftY(i);
      uint32_t bottomY = topY + sps->getSubPicHeight(i) - 1;
      subpicHeightInTiles[i] = m_ctuToTileRow[bottomY] + 1 - m_ctuToTileRow[topY];

      if (subpicHeightInTiles[i] == 1 && sps->getSubPicHeight(i) < m_tileRowHeight[m_ctuToTileRow[topY]] )
      {
        subpicHeightLessThanOneTileFlag[i] = 1;
      }
      else
      {
        subpicHeightLessThanOneTileFlag[i] = 0;
      }
    }

    for( int i = 0; i < m_numSlicesInPic; i++ )
    {
      CHECK(m_numSlicesInPic != sps->getNumSubPics(), "in single slice per subpic mode, number of slice and subpic shall be equal");
      m_sliceMap[ i ].initSliceMap();
      if (subpicHeightLessThanOneTileFlag[i])
      {
        m_sliceMap[i].addCtusToSlice(sps->getSubPicCtuTopLeftX(i), sps->getSubPicCtuTopLeftX(i) + sps->getSubPicWidth(i), 
                                     sps->getSubPicCtuTopLeftY(i), sps->getSubPicCtuTopLeftY(i) + sps->getSubPicHeight(i), m_picWidthInCtu);
      }
      else
      {
        tileX = m_ctuToTileCol[sps->getSubPicCtuTopLeftX(i)];
        tileY = m_ctuToTileRow[sps->getSubPicCtuTopLeftY(i)];
        for (uint32_t j = 0; j< subpicHeightInTiles[i]; j++)
        { 
          for (uint32_t k = 0; k < subpicWidthInTiles[i]; k++)
          {
            m_sliceMap[i].addCtusToSlice(getTileColumnBd(tileX + k), getTileColumnBd(tileX + k + 1), getTileRowBd(tileY + j), getTileRowBd(tileY + j + 1), m_picWidthInCtu);
          }
        }
      }
    }
    subpicWidthInTiles.clear();
    subpicHeightInTiles.clear();
    subpicHeightLessThanOneTileFlag.clear();
  }
  else
  {
    // allocate new memory for slice list
    CHECK(m_numSlicesInPic > MAX_SLICES, "Number of slices in picture exceeds valid range");
    m_sliceMap.resize( m_numSlicesInPic );
    // generate CTU maps for all rectangular slices in picture
    for( uint32_t i = 0; i < m_numSlicesInPic; i++ )
    {
      m_sliceMap[ i ].initSliceMap();

      // get position of first tile in slice
      tileX =  m_rectSlices[ i ].getTileIdx() % m_numTileCols;
      tileY =  m_rectSlices[ i ].getTileIdx() / m_numTileCols;

      // infer slice size for last slice in picture
      if( i == m_numSlicesInPic-1 )
      {
        m_rectSlices[ i ].setSliceWidthInTiles ( m_numTileCols - tileX );
        m_rectSlices[ i ].setSliceHeightInTiles( m_numTileRows - tileY );
        m_rectSlices[ i ].setNumSlicesInTile( 1 );
      }

      // set slice index
      m_sliceMap[ i ].setSliceID(i);

      // complete tiles within a single slice case
      if( m_rectSlices[ i ].getSliceWidthInTiles( ) > 1 || m_rectSlices[ i ].getSliceHeightInTiles( ) > 1)
      {
        for( uint32_t j = 0; j < m_rectSlices[ i ].getSliceHeightInTiles( ); j++ )
        {
          for( uint32_t k = 0; k < m_rectSlices[ i ].getSliceWidthInTiles( ); k++ )
          {
            m_sliceMap[ i ].addCtusToSlice( getTileColumnBd(tileX + k), getTileColumnBd(tileX + k +1),
                                            getTileRowBd(tileY + j), getTileRowBd(tileY + j +1), m_picWidthInCtu);
          }
        }
      }
      // multiple slices within a single tile case
      else
      {
        uint32_t  numSlicesInTile = m_rectSlices[ i ].getNumSlicesInTile( );

        ctuY = getTileRowBd( tileY );
        for( uint32_t j = 0; j < numSlicesInTile-1; j++ )
        {
          m_sliceMap[ i ].addCtusToSlice( getTileColumnBd(tileX), getTileColumnBd(tileX+1),
                                          ctuY, ctuY + m_rectSlices[ i ].getSliceHeightInCtu(), m_picWidthInCtu);
          ctuY += m_rectSlices[ i ].getSliceHeightInCtu();
          i++;
          m_sliceMap[ i ].initSliceMap();
          m_sliceMap[ i ].setSliceID(i);
        }

        // infer slice height for last slice in tile
        CHECK( ctuY >= getTileRowBd( tileY + 1 ), "Invalid rectangular slice signalling");
        m_rectSlices[ i ].setSliceHeightInCtu( getTileRowBd( tileY + 1 ) - ctuY );
        m_sliceMap[ i ].addCtusToSlice( getTileColumnBd(tileX), getTileColumnBd(tileX+1),
                                        ctuY, getTileRowBd( tileY + 1 ), m_picWidthInCtu);
      }
    }
  }
  // check for valid rectangular slice map
  checkSliceMap();
}

/**
- initialize mapping between subpicture and CTUs
*/
#if JVET_O1143_SUBPIC_BOUNDARY
void PPS::initSubPic(const SPS &sps)
{
  if (getSubPicIdMappingInPpsFlag())
  {
    // When signalled, the number of subpictures has to match in PPS and SPS
    CHECK (getNumSubPics() != sps.getNumSubPics(), "pps_num_subpics_minus1 shall be equal to sps_num_subpics_minus1");
  }
  else
  {
    // When not signalled  set the numer equal for convenient access
    setNumSubPics(sps.getNumSubPics());
  }

  CHECK(getNumSubPics() > MAX_NUM_SUB_PICS, "Number of sub-pictures in picture exceeds valid range");
  m_subPics.resize(getNumSubPics());
  // m_ctuSize,  m_picWidthInCtu, and m_picHeightInCtu might not be initialized yet.
  if (m_ctuSize == 0 || m_picWidthInCtu == 0 || m_picHeightInCtu == 0)
  {
    m_ctuSize = sps.getCTUSize();
    m_picWidthInCtu = (m_picWidthInLumaSamples + m_ctuSize - 1) / m_ctuSize;
    m_picHeightInCtu = (m_picHeightInLumaSamples + m_ctuSize - 1) / m_ctuSize;
  }
  for (int i=0; i< getNumSubPics(); i++)
  {
    m_subPics[i].setSubPicIdx(i);
#if JVET_Q0044_SLICE_IDX_WITH_SUBPICS
    if(sps.getSubPicIdMappingExplicitlySignalledFlag())
    {
      if(m_subPicIdMappingInPpsFlag)
      {
        m_subPics[i].setSubPicID(m_subPicId[i]);
      }
      else
      {
        m_subPics[i].setSubPicID(sps.getSubPicId(i));
      }
    }
    else
    {
      m_subPics[i].setSubPicID(i);
    }
#endif
    m_subPics[i].setSubPicCtuTopLeftX(sps.getSubPicCtuTopLeftX(i));
    m_subPics[i].setSubPicCtuTopLeftY(sps.getSubPicCtuTopLeftY(i));
    m_subPics[i].setSubPicWidthInCTUs(sps.getSubPicWidth(i));
    m_subPics[i].setSubPicHeightInCTUs(sps.getSubPicHeight(i));
    
    uint32_t firstCTU = sps.getSubPicCtuTopLeftY(i) * m_picWidthInCtu + sps.getSubPicCtuTopLeftX(i); 	
    m_subPics[i].setFirstCTUInSubPic(firstCTU);  
    uint32_t lastCTU = (sps.getSubPicCtuTopLeftY(i) + sps.getSubPicHeight(i) - 1) * m_picWidthInCtu + sps.getSubPicCtuTopLeftX(i) + sps.getSubPicWidth(i) - 1;
    m_subPics[i].setLastCTUInSubPic(lastCTU);
    
    uint32_t left = sps.getSubPicCtuTopLeftX(i) * m_ctuSize;
    m_subPics[i].setSubPicLeft(left);
    
    uint32_t right = std::min(m_picWidthInLumaSamples - 1, (sps.getSubPicCtuTopLeftX(i) + sps.getSubPicWidth(i)) * m_ctuSize - 1);
    m_subPics[i].setSubPicRight(right);
    
    m_subPics[i].setSubPicWidthInLumaSample(right - left + 1);

    uint32_t top = sps.getSubPicCtuTopLeftY(i) * m_ctuSize;
    m_subPics[i].setSubPicTop(top);
    
    uint32_t bottom = std::min(m_picHeightInLumaSamples - 1, (sps.getSubPicCtuTopLeftY(i) + sps.getSubPicHeight(i)) * m_ctuSize - 1);

    m_subPics[i].setSubPicHeightInLumaSample(bottom - top + 1);

    m_subPics[i].setSubPicBottom(bottom);
    
    m_subPics[i].clearCTUAddrList();
    
    if (m_numSlicesInPic == 1)
    {
      CHECK(getNumSubPics() != 1, "only one slice in picture, but number of subpic is not one");
      m_subPics[i].addAllCtusInPicToSubPic(0, getPicWidthInCtu(), 0, getPicHeightInCtu(), getPicWidthInCtu());
#if JVET_Q0044_SLICE_IDX_WITH_SUBPICS
      m_subPics[i].setNumSlicesInSubPic(1);
#endif
    }
    else
    {
#if JVET_Q0044_SLICE_IDX_WITH_SUBPICS
      int numSlicesInSubPic = 0;
#endif
      for (int j = 0; j < m_numSlicesInPic; j++)
      {
        uint32_t ctu = m_sliceMap[j].getCtuAddrInSlice(0);
        uint32_t ctu_x = ctu % m_picWidthInCtu;
        uint32_t ctu_y = ctu / m_picWidthInCtu;
        if (ctu_x >= sps.getSubPicCtuTopLeftX(i) &&
          ctu_x < (sps.getSubPicCtuTopLeftX(i) + sps.getSubPicWidth(i)) &&
          ctu_y >= sps.getSubPicCtuTopLeftY(i) &&
          ctu_y < (sps.getSubPicCtuTopLeftY(i) + sps.getSubPicHeight(i)))  
        {
          // add ctus in a slice to the subpicture it belongs to
          m_subPics[i].addCTUsToSubPic(m_sliceMap[j].getCtuAddrList());
#if JVET_Q0044_SLICE_IDX_WITH_SUBPICS
	  numSlicesInSubPic++;
#endif
        }
      }
#if JVET_Q0044_SLICE_IDX_WITH_SUBPICS
      m_subPics[i].setNumSlicesInSubPic(numSlicesInSubPic);
#endif
    }
    m_subPics[i].setTreatedAsPicFlag(sps.getSubPicTreatedAsPicFlag(i));
    m_subPics[i].setloopFilterAcrossEnabledFlag(sps.getLoopFilterAcrossSubpicEnabledFlag(i));
  }
}

const SubPic& PPS::getSubPicFromPos(const Position& pos)  const
{
  for (int i = 0; i< m_numSubPics; i++)
  {
    if (m_subPics[i].isContainingPos(pos))
    {
      return m_subPics[i];
    }
  }
  return m_subPics[0];
}

const SubPic&  PPS::getSubPicFromCU(const CodingUnit& cu) const 
{
  const Position lumaPos = cu.Y().valid() ? cu.Y().pos() : recalcPosition(cu.chromaFormat, cu.chType, CHANNEL_TYPE_LUMA, cu.blocks[cu.chType].pos());
  return getSubPicFromPos(lumaPos);
}
#endif

#if JVET_Q0044_SLICE_IDX_WITH_SUBPICS
uint32_t PPS::getSubPicIdxFromSubPicId( uint32_t subPicId ) const
{
  for (int i = 0; i < m_numSubPics; i++)
  {
    if(m_subPics[i].getSubPicID() == subPicId)
    {
      return i;
    }
  }
  return 0;
}
#endif

void PPS::initRasterSliceMap( std::vector<uint32_t> numTilesInSlice )
{
  uint32_t tileIdx = 0;
  setNumSlicesInPic( (uint32_t) numTilesInSlice.size() );

  // allocate new memory for slice list
  CHECK(m_numSlicesInPic > MAX_SLICES, "Number of slices in picture exceeds valid range");
  m_sliceMap.resize( m_numSlicesInPic );

  for( uint32_t sliceIdx = 0; sliceIdx < numTilesInSlice.size(); sliceIdx++ ) 
  {
    m_sliceMap[sliceIdx].initSliceMap();
    m_sliceMap[sliceIdx].setSliceID( tileIdx );
    m_sliceMap[sliceIdx].setNumTilesInSlice( numTilesInSlice[sliceIdx] );
    for( uint32_t idx = 0; idx < numTilesInSlice[sliceIdx]; idx++ )
    {
      uint32_t tileX = tileIdx % getNumTileColumns();
      uint32_t tileY = tileIdx / getNumTileColumns();
      CHECK(tileY >= getNumTileRows(), "Number of tiles in slice exceeds the remaining number of tiles in picture");

      m_sliceMap[sliceIdx].addCtusToSlice(getTileColumnBd(tileX), getTileColumnBd(tileX + 1),
                                          getTileRowBd(tileY), getTileRowBd(tileY + 1), 
                                          getPicWidthInCtu());
      tileIdx++;
    }
  }

  // check for valid raster-scan slice map
  checkSliceMap();
}

/**
 - check if slice map covers the entire picture without skipping or duplicating any CTU positions
 */
void PPS::checkSliceMap()
{
  uint32_t i;
  std::vector<uint32_t>  ctuList, sliceList;
  uint32_t picSizeInCtu = getPicWidthInCtu() * getPicHeightInCtu();
  for( i = 0; i < m_numSlicesInPic; i++ )
  {
    sliceList = m_sliceMap[ i ].getCtuAddrList();
    ctuList.insert( ctuList.end(), sliceList.begin(), sliceList.end() );
  }  
  CHECK( ctuList.size() < picSizeInCtu, "Slice map contains too few CTUs");
  CHECK( ctuList.size() > picSizeInCtu, "Slice map contains too many CTUs");
  std::sort( ctuList.begin(), ctuList.end() );   
  for( i = 1; i < ctuList.size(); i++ )
  {
    CHECK( ctuList[i] > ctuList[i-1]+1, "CTU missing in slice map");
    CHECK( ctuList[i] == ctuList[i-1],  "CTU duplicated in slice map");
  }
}

APS::APS()
: m_APSId(0)
, m_temporalId( 0 )
, m_layerId( 0 )
{
}

APS::~APS()
{
}

ReferencePictureList::ReferencePictureList( const bool interLayerPicPresentFlag )
  : m_numberOfShorttermPictures(0)
  , m_numberOfLongtermPictures(0)
  , m_numberOfActivePictures(MAX_INT)
  , m_ltrp_in_slice_header_flag(0)
  , m_interLayerPresentFlag( interLayerPicPresentFlag )
  , m_numberOfInterLayerPictures( 0 )
{
  ::memset(m_isLongtermRefPic, 0, sizeof(m_isLongtermRefPic));
  ::memset(m_refPicIdentifier, 0, sizeof(m_refPicIdentifier));
  ::memset(m_POC, 0, sizeof(m_POC));
  ::memset( m_isInterLayerRefPic, 0, sizeof( m_isInterLayerRefPic ) );
  ::memset( m_interLayerRefPicIdx, 0, sizeof( m_interLayerRefPicIdx ) );
}

ReferencePictureList::~ReferencePictureList()
{
}

void ReferencePictureList::setRefPicIdentifier( int idx, int identifier, bool isLongterm, bool isInterLayerRefPic, int interLayerIdx )
{
  m_refPicIdentifier[idx] = identifier;
  m_isLongtermRefPic[idx] = isLongterm;

  m_deltaPocMSBPresentFlag[idx] = false;
  m_deltaPOCMSBCycleLT[idx] = 0;

  m_isInterLayerRefPic[idx] = isInterLayerRefPic;
  m_interLayerRefPicIdx[idx] = interLayerIdx;
}

int ReferencePictureList::getRefPicIdentifier(int idx) const
{
  return m_refPicIdentifier[idx];
}


bool ReferencePictureList::isRefPicLongterm(int idx) const
{
  return m_isLongtermRefPic[idx];
}

void ReferencePictureList::setNumberOfShorttermPictures(int numberOfStrp)
{
  m_numberOfShorttermPictures = numberOfStrp;
}

int ReferencePictureList::getNumberOfShorttermPictures() const
{
  return m_numberOfShorttermPictures;
}

void ReferencePictureList::setNumberOfLongtermPictures(int numberOfLtrp)
{
  m_numberOfLongtermPictures = numberOfLtrp;
}

int ReferencePictureList::getNumberOfLongtermPictures() const
{
  return m_numberOfLongtermPictures;
}

void ReferencePictureList::setPOC(int idx, int POC)
{
  m_POC[idx] = POC;
}

int ReferencePictureList::getPOC(int idx) const
{
  return m_POC[idx];
}

void ReferencePictureList::setNumberOfActivePictures(int numberActive)
{
  m_numberOfActivePictures = numberActive;
}

int ReferencePictureList::getNumberOfActivePictures() const
{
  return m_numberOfActivePictures;
}

void ReferencePictureList::printRefPicInfo() const
{
  //DTRACE(g_trace_ctx, D_RPSINFO, "RefPics = { ");
  printf("RefPics = { ");
  int numRefPic = getNumberOfShorttermPictures() + getNumberOfLongtermPictures();
  for (int ii = 0; ii < numRefPic; ii++)
  {
    //DTRACE(g_trace_ctx, D_RPSINFO, "%d%s ", m_refPicIdentifier[ii], (m_isLongtermRefPic[ii] == 1) ? "[LT]" : "[ST]");
    printf("%d%s ", m_refPicIdentifier[ii], (m_isLongtermRefPic[ii] == 1) ? "[LT]" : "[ST]");
  }
  //DTRACE(g_trace_ctx, D_RPSINFO, "}\n");
  printf("}\n");
}

ScalingList::ScalingList()
{
  m_disableScalingMatrixForLfnstBlks = true;
#if JVET_Q0505_CHROAM_QM_SIGNALING_400
  m_chromaScalingListPresentFlag = true;
#endif
  for (uint32_t scalingListId = 0; scalingListId < 28; scalingListId++)
  {
    int matrixSize = (scalingListId < SCALING_LIST_1D_START_4x4) ? 2 : (scalingListId < SCALING_LIST_1D_START_8x8) ? 4 : 8;
    m_scalingListCoef[scalingListId].resize(matrixSize*matrixSize);
  }
}

/** set default quantization matrix to array
*/
void ScalingList::setDefaultScalingList()
{
  for (uint32_t scalingListId = 0; scalingListId < 28; scalingListId++)
  {
    processDefaultMatrix(scalingListId);
  }
}
/** check if use default quantization matrix
 * \returns true if the scaling list is not equal to the default quantization matrix
*/
bool ScalingList::isNotDefaultScalingList()
{
  bool isAllDefault = true;
  for (uint32_t scalingListId = 0; scalingListId < 28; scalingListId++)
  {
    int matrixSize = (scalingListId < SCALING_LIST_1D_START_4x4) ? 2 : (scalingListId < SCALING_LIST_1D_START_8x8) ? 4 : 8;
    if (scalingListId < SCALING_LIST_1D_START_16x16)
    {
      if (::memcmp(getScalingListAddress(scalingListId), getScalingListDefaultAddress(scalingListId), sizeof(int) * matrixSize * matrixSize))
      {
        isAllDefault = false;
        break;
      }
    }
    else
    {
      if ((::memcmp(getScalingListAddress(scalingListId), getScalingListDefaultAddress(scalingListId), sizeof(int) * MAX_MATRIX_COEF_NUM)) || (getScalingListDC(scalingListId) != 16))
      {
        isAllDefault = false;
        break;
      }
    }
    if (!isAllDefault) break;
  }

  return !isAllDefault;
}

/** get scaling matrix from RefMatrixID
 * \param sizeId    size index
 * \param listId    index of input matrix
 * \param refListId index of reference matrix
 */
int ScalingList::lengthUvlc(int uiCode)
{
  if (uiCode < 0) printf("Error UVLC! \n");

  int uiLength = 1;
  int uiTemp = ++uiCode;

  CHECK(!uiTemp, "Integer overflow");

  while (1 != uiTemp)
  {
    uiTemp >>= 1;
    uiLength += 2;
  }
  return (uiLength >> 1) + ((uiLength + 1) >> 1);
}
int ScalingList::lengthSvlc(int uiCode)
{
  uint32_t uiCode2 = uint32_t(uiCode <= 0 ? (-uiCode) << 1 : (uiCode << 1) - 1);
  int uiLength = 1;
  int uiTemp = ++uiCode2;

  CHECK(!uiTemp, "Integer overflow");

  while (1 != uiTemp)
  {
    uiTemp >>= 1;
    uiLength += 2;
  }
  return (uiLength >> 1) + ((uiLength + 1) >> 1);
}
void ScalingList::codePredScalingList(int* scalingList, const int* scalingListPred, int scalingListDC, int scalingListPredDC, int scalingListId, int& bitsCost) //sizeId, listId is current to-be-coded matrix idx
{
  int deltaValue = 0;
  int matrixSize = (scalingListId < SCALING_LIST_1D_START_4x4) ? 2 : (scalingListId < SCALING_LIST_1D_START_8x8) ? 4 : 8;
  int coefNum = matrixSize*matrixSize;
  ScanElement *scan = g_scanOrder[SCAN_UNGROUPED][SCAN_DIAG][gp_sizeIdxInfo->idxFrom(matrixSize)][gp_sizeIdxInfo->idxFrom(matrixSize)];
  int nextCoef = 0;

  int8_t data;
  const int *src = scalingList;
  const int *srcPred = scalingListPred;
  if (scalingListDC!=-1 && scalingListPredDC!=-1)
  {
    bitsCost += lengthSvlc((int8_t)(scalingListDC - scalingListPredDC - nextCoef));
    nextCoef =  scalingListDC - scalingListPredDC;
  }
  else if ((scalingListDC != -1 && scalingListPredDC == -1))
  {
    bitsCost += lengthSvlc((int8_t)(scalingListDC - srcPred[scan[0].idx] - nextCoef));
    nextCoef =  scalingListDC - srcPred[scan[0].idx];
  }
  else if ((scalingListDC == -1 && scalingListPredDC == -1))
  {
  }
  else
  {
    printf("Predictor DC mismatch! \n");
  }
  for (int i = 0; i < coefNum; i++)
  {
    if (scalingListId >= SCALING_LIST_1D_START_64x64 && scan[i].x >= 4 && scan[i].y >= 4)
      continue;
    deltaValue = (src[scan[i].idx] - srcPred[scan[i].idx]);
    data = (int8_t)(deltaValue - nextCoef);
    nextCoef = deltaValue;

    bitsCost += lengthSvlc(data);
  }
}
void ScalingList::codeScalingList(int* scalingList, int scalingListDC, int scalingListId, int& bitsCost) //sizeId, listId is current to-be-coded matrix idx
{
  int matrixSize = (scalingListId < SCALING_LIST_1D_START_4x4) ? 2 : (scalingListId < SCALING_LIST_1D_START_8x8) ? 4 : 8;
  int coefNum = matrixSize * matrixSize;
  ScanElement *scan = g_scanOrder[SCAN_UNGROUPED][SCAN_DIAG][gp_sizeIdxInfo->idxFrom(matrixSize)][gp_sizeIdxInfo->idxFrom(matrixSize)];
  int nextCoef = SCALING_LIST_START_VALUE;
  int8_t data;
  const int *src = scalingList;

  if (scalingListId >= SCALING_LIST_1D_START_16x16)
  {
    bitsCost += lengthSvlc(int8_t(getScalingListDC(scalingListId) - nextCoef));
    nextCoef = getScalingListDC(scalingListId);
  }

  for (int i = 0; i < coefNum; i++)
  {
    if (scalingListId >= SCALING_LIST_1D_START_64x64 && scan[i].x >= 4 && scan[i].y >= 4)
      continue;
    data = int8_t(src[scan[i].idx] - nextCoef);
    nextCoef = src[scan[i].idx];

    bitsCost += lengthSvlc(data);
  }
}
void ScalingList::CheckBestPredScalingList(int scalingListId, int predListId, int& BitsCount)
{
  //check previously coded matrix as a predictor, code "lengthUvlc" function
  int *scalingList = getScalingListAddress(scalingListId);
  const int *scalingListPred = (scalingListId == predListId) ? ((predListId < SCALING_LIST_1D_START_8x8) ? g_quantTSDefault4x4 : g_quantIntraDefault8x8) : getScalingListAddress(predListId);
  int scalingListDC = (scalingListId >= SCALING_LIST_1D_START_16x16) ? getScalingListDC(scalingListId) : -1;
  int scalingListPredDC = (predListId >= SCALING_LIST_1D_START_16x16) ? ((scalingListId == predListId) ? 16 : getScalingListDC(predListId)) : -1;

  int bitsCost = 0;
  int matrixSize = (scalingListId < SCALING_LIST_1D_START_4x4) ? 2 : (scalingListId < SCALING_LIST_1D_START_8x8) ? 4 : 8;
  int predMatrixSize = (predListId < SCALING_LIST_1D_START_4x4) ? 2 : (predListId < SCALING_LIST_1D_START_8x8) ? 4 : 8;

  if (matrixSize != predMatrixSize) printf("Predictor size mismatch! \n");

  bitsCost = 2 + lengthUvlc(scalingListId - predListId);
  //copy-flag + predictor-mode-flag + deltaListId
  codePredScalingList(scalingList, scalingListPred, scalingListDC, scalingListPredDC, scalingListId, bitsCost);
  BitsCount = bitsCost;
}
void ScalingList::processRefMatrix(uint32_t scalinListId, uint32_t refListId)
{
  int matrixSize = (scalinListId < SCALING_LIST_1D_START_4x4) ? 2 : (scalinListId < SCALING_LIST_1D_START_8x8) ? 4 : 8;
  ::memcpy(getScalingListAddress(scalinListId), ((scalinListId == refListId) ? getScalingListDefaultAddress(refListId) : getScalingListAddress(refListId)), sizeof(int)*matrixSize*matrixSize);
}
void ScalingList::checkPredMode(uint32_t scalingListId)
{
  int bestBitsCount = MAX_INT;
  int BitsCount = 2;
  setScalingListPreditorModeFlag(scalingListId, false);
  codeScalingList(getScalingListAddress(scalingListId), ((scalingListId >= SCALING_LIST_1D_START_16x16) ? getScalingListDC(scalingListId) : -1), scalingListId, BitsCount);
  bestBitsCount = BitsCount;

  for (int predListIdx = (int)scalingListId; predListIdx >= 0; predListIdx--)
  {

    int matrixSize = (scalingListId < SCALING_LIST_1D_START_4x4) ? 2 : (scalingListId < SCALING_LIST_1D_START_8x8) ? 4 : 8;
    int predMatrixSize = (predListIdx < SCALING_LIST_1D_START_4x4) ? 2 : (predListIdx < SCALING_LIST_1D_START_8x8) ? 4 : 8;
    if (((scalingListId == SCALING_LIST_1D_START_2x2 || scalingListId == SCALING_LIST_1D_START_4x4 || scalingListId == SCALING_LIST_1D_START_8x8) && predListIdx != (int)scalingListId) || matrixSize != predMatrixSize)
      continue;
    const int* refScalingList = (scalingListId == predListIdx) ? getScalingListDefaultAddress(predListIdx) : getScalingListAddress(predListIdx);
    const int refDC = (predListIdx < SCALING_LIST_1D_START_16x16) ? refScalingList[0] : (scalingListId == predListIdx) ? 16 : getScalingListDC(predListIdx);
    if (!::memcmp(getScalingListAddress(scalingListId), refScalingList, sizeof(int)*matrixSize*matrixSize) // check value of matrix
      // check DC value
      && (scalingListId < SCALING_LIST_1D_START_16x16 || getScalingListDC(scalingListId) == refDC))
    {
      //copy mode
      setRefMatrixId(scalingListId, predListIdx);
      setScalingListCopyModeFlag(scalingListId, true);
      setScalingListPreditorModeFlag(scalingListId, false);
      return;
    }
    else
    {
      //predictor mode
      //use previously coded matrix as a predictor
      CheckBestPredScalingList(scalingListId, predListIdx, BitsCount);
      if (BitsCount < bestBitsCount)
      {
        bestBitsCount = BitsCount;
        setScalingListCopyModeFlag(scalingListId, false);
        setScalingListPreditorModeFlag(scalingListId, true);
        setRefMatrixId(scalingListId, predListIdx);
      }
    }
  }
  setScalingListCopyModeFlag(scalingListId, false);
}

static void outputScalingListHelp(std::ostream &os)
{
  os << "The scaling list file specifies all matrices and their DC values; none can be missing,\n"
         "but their order is arbitrary.\n\n"
         "The matrices are specified by:\n"
         "<matrix name><unchecked data>\n"
         "  <value>,<value>,<value>,....\n\n"
         "  Line-feeds can be added arbitrarily between values, and the number of values needs to be\n"
         "  at least the number of entries for the matrix (superfluous entries are ignored).\n"
         "  The <unchecked data> is text on the same line as the matrix that is not checked\n"
         "  except to ensure that the matrix name token is unique. It is recommended that it is ' ='\n"
         "  The values in the matrices are the absolute values (0-255), not the delta values as\n"
         "  exchanged between the encoder and decoder\n\n"
         "The DC values (for matrix sizes larger than 8x8) are specified by:\n"
         "<matrix name>_DC<unchecked data>\n"
         "  <value>\n";

  os << "The permitted matrix names are:\n";
  for (uint32_t sizeIdc = SCALING_LIST_2x2; sizeIdc <= SCALING_LIST_64x64; sizeIdc++)
  {
    for (uint32_t listIdc = 0; listIdc < SCALING_LIST_NUM; listIdc++)
    {
      if (!(((sizeIdc == SCALING_LIST_64x64) && (listIdc % (SCALING_LIST_NUM / SCALING_LIST_PRED_MODES) != 0)) || ((sizeIdc == SCALING_LIST_2x2) && (listIdc % (SCALING_LIST_NUM / SCALING_LIST_PRED_MODES) == 0))))
      {
        os << "  " << MatrixType[sizeIdc][listIdc] << '\n';
      }
    }
  }
}

void ScalingList::outputScalingLists(std::ostream &os) const
{
  int scalingListId = 0;
  for (uint32_t sizeIdc = SCALING_LIST_2x2; sizeIdc <= SCALING_LIST_64x64; sizeIdc++)
  {
    const uint32_t size = (sizeIdc == 1) ? 2 : ((sizeIdc == 2) ? 4 : 8);
    for(uint32_t listIdc = 0; listIdc < SCALING_LIST_NUM; listIdc++)
    {
      if (!((sizeIdc== SCALING_LIST_64x64 && listIdc % (SCALING_LIST_NUM / SCALING_LIST_PRED_MODES) != 0) || (sizeIdc == SCALING_LIST_2x2 && listIdc < 4)))
      {
        const int *src = getScalingListAddress(scalingListId);
        os << (MatrixType[sizeIdc][listIdc]) << " =\n  ";
        for(uint32_t y=0; y<size; y++)
        {
          for(uint32_t x=0; x<size; x++, src++)
          {
            os << std::setw(3) << (*src) << ", ";
          }
          os << (y+1<size?"\n  ":"\n");
        }
        if(sizeIdc > SCALING_LIST_8x8)
        {
          os << MatrixType_DC[sizeIdc][listIdc] << " = \n  " << std::setw(3) << getScalingListDC(scalingListId) << "\n";
        }
        os << "\n";
        scalingListId++;
      }
    }
  }
}

bool ScalingList::xParseScalingList(const std::string &fileName)
{
  static const int LINE_SIZE=1024;
  FILE *fp = NULL;
  char line[LINE_SIZE];

  if (fileName.empty())
  {
    msg( ERROR, "Error: no scaling list file specified. Help on scaling lists being output\n");
    outputScalingListHelp(std::cout);
    std::cout << "\n\nExample scaling list file using default values:\n\n";
    outputScalingLists(std::cout);
    return true;
  }
  else if ((fp = fopen(fileName.c_str(),"r")) == (FILE*)NULL)
  {
    msg( ERROR, "Error: cannot open scaling list file %s for reading\n", fileName.c_str());
    return true;
  }

  int scalingListId = 0;
  for (uint32_t sizeIdc = SCALING_LIST_2x2; sizeIdc <= SCALING_LIST_64x64; sizeIdc++)//2x2-128x128
  {
    const uint32_t size = std::min(MAX_MATRIX_COEF_NUM,(int)g_scalingListSize[sizeIdc]);

    for(uint32_t listIdc = 0; listIdc < SCALING_LIST_NUM; listIdc++)
    {

      if ((sizeIdc == SCALING_LIST_64x64 && listIdc % (SCALING_LIST_NUM / SCALING_LIST_PRED_MODES) != 0) || (sizeIdc == SCALING_LIST_2x2 && listIdc < 4))
      {
        continue;
      }
      else
      {
        int * const src = getScalingListAddress(scalingListId);
        {
          fseek(fp, 0, SEEK_SET);
          bool bFound=false;
          while ((!feof(fp)) && (!bFound))
          {
            char *ret = fgets(line, LINE_SIZE, fp);
            char *findNamePosition= ret==NULL ? NULL : strstr(line, MatrixType[sizeIdc][listIdc]);
            // This could be a match against the DC string as well, so verify it isn't
            if (findNamePosition!= NULL && (MatrixType_DC[sizeIdc][listIdc]==NULL || strstr(line, MatrixType_DC[sizeIdc][listIdc])==NULL))
            {
              bFound=true;
            }
          }
          if (!bFound)
          {
            msg( ERROR, "Error: cannot find Matrix %s from scaling list file %s\n", MatrixType[sizeIdc][listIdc], fileName.c_str());
            return true;

          }
        }
        for (uint32_t i=0; i<size; i++)
        {
          int data;
          if (fscanf(fp, "%d,", &data)!=1)
          {
            msg( ERROR, "Error: cannot read value #%d for Matrix %s from scaling list file %s at file position %ld\n", i, MatrixType[sizeIdc][listIdc], fileName.c_str(), ftell(fp));
            return true;
          }
          if (data<0 || data>255)
          {
            msg( ERROR, "Error: QMatrix entry #%d of value %d for Matrix %s from scaling list file %s at file position %ld is out of range (0 to 255)\n", i, data, MatrixType[sizeIdc][listIdc], fileName.c_str(), ftell(fp));
            return true;
          }
          src[i] = data;
        }

        //set DC value for default matrix check
        setScalingListDC(scalingListId, src[0]);

        if(sizeIdc > SCALING_LIST_8x8)
        {
          {
            fseek(fp, 0, SEEK_SET);
            bool bFound=false;
            while ((!feof(fp)) && (!bFound))
            {
              char *ret = fgets(line, LINE_SIZE, fp);
              char *findNamePosition= ret==NULL ? NULL : strstr(line, MatrixType_DC[sizeIdc][listIdc]);
              if (findNamePosition!= NULL)
              {
                // This won't be a match against the non-DC string.
                bFound=true;
              }
            }
            if (!bFound)
            {
              msg( ERROR, "Error: cannot find DC Matrix %s from scaling list file %s\n", MatrixType_DC[sizeIdc][listIdc], fileName.c_str());
              return true;
            }
          }
          int data;
          if (fscanf(fp, "%d,", &data)!=1)
          {
            msg( ERROR, "Error: cannot read DC %s from scaling list file %s at file position %ld\n", MatrixType_DC[sizeIdc][listIdc], fileName.c_str(), ftell(fp));
            return true;
          }
          if (data<0 || data>255)
          {
            msg( ERROR, "Error: DC value %d for Matrix %s from scaling list file %s at file position %ld is out of range (0 to 255)\n", data, MatrixType[sizeIdc][listIdc], fileName.c_str(), ftell(fp));
            return true;
          }
          //overwrite DC value when size of matrix is larger than 16x16
          setScalingListDC(scalingListId, data);
        }
      }
      scalingListId++;
    }
  }
//  std::cout << "\n\nRead scaling lists of:\n\n";
//  outputScalingLists(std::cout);

  fclose(fp);
  return false;
}


/** get default address of quantization matrix
 * \param sizeId size index
 * \param listId list index
 * \returns pointer of quantization matrix
 */
const int* ScalingList::getScalingListDefaultAddress(uint32_t scalingListId)
{
  const int *src = 0;
  int sizeId = (scalingListId < SCALING_LIST_1D_START_8x8) ? 2 : 3;
  switch (sizeId)
  {
    case SCALING_LIST_1x1:
    case SCALING_LIST_2x2:
    case SCALING_LIST_4x4:
      src = g_quantTSDefault4x4;
      break;
    case SCALING_LIST_8x8:
    case SCALING_LIST_16x16:
    case SCALING_LIST_32x32:
    case SCALING_LIST_64x64:
    case SCALING_LIST_128x128:
      src = g_quantInterDefault8x8;
      break;
    default:
      THROW( "Invalid scaling list" );
      src = NULL;
      break;
  }
  return src;
}

/** process of default matrix
 * \param sizeId size index
 * \param listId index of input matrix
 */
void ScalingList::processDefaultMatrix(uint32_t scalingListId)
{
  int matrixSize = (scalingListId < SCALING_LIST_1D_START_4x4) ? 2 : (scalingListId < SCALING_LIST_1D_START_8x8) ? 4 : 8;
  ::memcpy(getScalingListAddress(scalingListId), getScalingListDefaultAddress(scalingListId), sizeof(int)*matrixSize*matrixSize);
  setScalingListDC(scalingListId, SCALING_LIST_DC);
}

/** check DC value of matrix for default matrix signaling
 */
void ScalingList::checkDcOfMatrix()
{
  for (uint32_t scalingListId = 0; scalingListId < 28; scalingListId++)
  {
    //check default matrix?
    if (getScalingListDC(scalingListId) == 0)
    {
      processDefaultMatrix(scalingListId);
    }
  }
}

#if JVET_Q0505_CHROAM_QM_SIGNALING_400
bool ScalingList::isLumaScalingList( int scalingListId) const
{
  return (scalingListId % MAX_NUM_COMPONENT == SCALING_LIST_1D_START_4x4 || scalingListId == SCALING_LIST_1D_START_64x64 + 1);
}
#endif

uint32_t PreCalcValues::getValIdx( const Slice &slice, const ChannelType chType ) const
{
  return slice.isIntra() ? ( ISingleTree ? 0 : ( chType << 1 ) ) : 1;
}

uint32_t PreCalcValues::getMaxBtDepth( const Slice &slice, const ChannelType chType ) const
{
  if ( slice.getPicHeader()->getSplitConsOverrideFlag() )    
    return slice.getPicHeader()->getMaxMTTHierarchyDepth( slice.getSliceType(), ISingleTree ? CHANNEL_TYPE_LUMA : chType);
  else
  return maxBtDepth[getValIdx( slice, chType )];
}

uint32_t PreCalcValues::getMinBtSize( const Slice &slice, const ChannelType chType ) const
{
  return minBtSize[getValIdx( slice, chType )];
}

uint32_t PreCalcValues::getMaxBtSize( const Slice &slice, const ChannelType chType ) const
{
  if (slice.getPicHeader()->getSplitConsOverrideFlag())
    return slice.getPicHeader()->getMaxBTSize( slice.getSliceType(), ISingleTree ? CHANNEL_TYPE_LUMA : chType);
  else
    return maxBtSize[getValIdx(slice, chType)];
}

uint32_t PreCalcValues::getMinTtSize( const Slice &slice, const ChannelType chType ) const
{
  return minTtSize[getValIdx( slice, chType )];
}

uint32_t PreCalcValues::getMaxTtSize( const Slice &slice, const ChannelType chType ) const
{
  if (slice.getPicHeader()->getSplitConsOverrideFlag())
    return slice.getPicHeader()->getMaxTTSize( slice.getSliceType(), ISingleTree ? CHANNEL_TYPE_LUMA : chType);
  else
  return maxTtSize[getValIdx( slice, chType )];
}
uint32_t PreCalcValues::getMinQtSize( const Slice &slice, const ChannelType chType ) const
{
  if (slice.getPicHeader()->getSplitConsOverrideFlag())
    return slice.getPicHeader()->getMinQTSize( slice.getSliceType(), ISingleTree ? CHANNEL_TYPE_LUMA : chType);
  else
  return minQtSize[getValIdx( slice, chType )];
}

void Slice::scaleRefPicList( Picture *scaledRefPic[ ], PicHeader *picHeader, APS** apss, APS* lmcsAps, APS* scalingListAps, const bool isDecoder )
{
  int i;
  const SPS* sps = getSPS();
  const PPS* pps = getPPS();

  bool refPicIsSameRes = false;
   
  // this is needed for IBC
  m_pcPic->unscaledPic = m_pcPic;

  if( m_eSliceType == I_SLICE )
  {
    return;
  }

  freeScaledRefPicList( scaledRefPic );

  for( int refList = 0; refList < NUM_REF_PIC_LIST_01; refList++ )
  {
    if( refList == 1 && m_eSliceType != B_SLICE )
    {
      continue;
    }

    for( int rIdx = 0; rIdx < m_aiNumRefIdx[refList]; rIdx++ )
    {
      // if rescaling is needed, otherwise just reuse the original picture pointer; it is needed for motion field, otherwise motion field requires a copy as well
      // reference resampling for the whole picture is not applied at decoder

      int xScale, yScale;
      CU::getRprScaling( sps, pps, m_apcRefPicList[refList][rIdx], xScale, yScale );
      m_scalingRatio[refList][rIdx] = std::pair<int, int>( xScale, yScale );

#if JVET_Q0487_SCALING_WINDOW_ISSUES
      if( m_apcRefPicList[refList][rIdx]->isRefScaled( pps ) == false )
#else
      if( m_scalingRatio[refList][rIdx] == SCALE_1X && pps->getPicWidthInLumaSamples() == m_apcRefPicList[refList][rIdx]->getPicWidthInLumaSamples() && pps->getPicHeightInLumaSamples() == m_apcRefPicList[refList][rIdx]->getPicHeightInLumaSamples() )
#endif
      {
        refPicIsSameRes = true;
      }

      if( m_scalingRatio[refList][rIdx] == SCALE_1X || isDecoder )
      {
        m_scaledRefPicList[refList][rIdx] = m_apcRefPicList[refList][rIdx];
      }
      else
      {
        int poc = m_apcRefPicList[refList][rIdx]->getPOC();
        int layerId = m_apcRefPicList[refList][rIdx]->layerId;

        // check whether the reference picture has already been scaled
        for( i = 0; i < MAX_NUM_REF; i++ )
        {
          if( scaledRefPic[i] != nullptr && scaledRefPic[i]->poc == poc && scaledRefPic[i]->layerId == layerId )
          {
            break;
          }
        }

        if( i == MAX_NUM_REF )
        {
          int j;
          // search for unused Picture structure in scaledRefPic
          for( j = 0; j < MAX_NUM_REF; j++ )
          {
            if( scaledRefPic[j] == nullptr )
            {
              break;
            }
          }

          CHECK( j >= MAX_NUM_REF, "scaledRefPic can not hold all reference pictures!" );

          if( j >= MAX_NUM_REF )
          {
            j = 0;
          }

          if( scaledRefPic[j] == nullptr )
          {
            scaledRefPic[j] = new Picture;

            scaledRefPic[j]->setBorderExtension( false );
            scaledRefPic[j]->reconstructed = false;
            scaledRefPic[j]->referenced = true;

            scaledRefPic[j]->finalInit( m_pcPic->cs->vps, *sps, *pps, picHeader, apss, lmcsAps, scalingListAps );

            scaledRefPic[j]->poc = NOT_VALID;

            scaledRefPic[j]->create( sps->getChromaFormatIdc(), Size( pps->getPicWidthInLumaSamples(), pps->getPicHeightInLumaSamples() ), sps->getMaxCUWidth(), sps->getMaxCUWidth() + 16, isDecoder, layerId );
          }

          scaledRefPic[j]->poc = poc;
          scaledRefPic[j]->longTerm = m_apcRefPicList[refList][rIdx]->longTerm;

          // rescale the reference picture
          const bool downsampling = m_apcRefPicList[refList][rIdx]->getRecoBuf().Y().width >= scaledRefPic[j]->getRecoBuf().Y().width && m_apcRefPicList[refList][rIdx]->getRecoBuf().Y().height >= scaledRefPic[j]->getRecoBuf().Y().height;
          Picture::rescalePicture( m_scalingRatio[refList][rIdx], 
                                   m_apcRefPicList[refList][rIdx]->getRecoBuf(), m_apcRefPicList[refList][rIdx]->slices[0]->getPPS()->getScalingWindow(), 
                                   scaledRefPic[j]->getRecoBuf(), pps->getScalingWindow(), 
                                   sps->getChromaFormatIdc(), sps->getBitDepths(), true, downsampling,
                                   sps->getHorCollocatedChromaFlag(), sps->getVerCollocatedChromaFlag() );
          scaledRefPic[j]->extendPicBorder();

          m_scaledRefPicList[refList][rIdx] = scaledRefPic[j];
        }
        else
        {
          m_scaledRefPicList[refList][rIdx] = scaledRefPic[i];
        }
      }
    }
  }

  // make the scaled reference picture list as the default reference picture list
  for( int refList = 0; refList < NUM_REF_PIC_LIST_01; refList++ )
  {
    if( refList == 1 && m_eSliceType != B_SLICE )
    {
      continue;
    }

    for( int rIdx = 0; rIdx < m_aiNumRefIdx[refList]; rIdx++ )
    {
      m_savedRefPicList[refList][rIdx] = m_apcRefPicList[refList][rIdx];
      m_apcRefPicList[refList][rIdx] = m_scaledRefPicList[refList][rIdx];

      // allow the access of the unscaled version in xPredInterBlk()
      m_apcRefPicList[refList][rIdx]->unscaledPic = m_savedRefPicList[refList][rIdx];
    }
  }
  
  //Make sure that TMVP is disabled when there are no reference pictures with the same resolution
  if(!refPicIsSameRes)
  {
    CHECK(getPicHeader()->getEnableTMVPFlag() != 0, "TMVP cannot be enabled in pictures that have no reference pictures with the same resolution")
  }
}

void Slice::freeScaledRefPicList( Picture *scaledRefPic[] )
{
  if( m_eSliceType == I_SLICE )
  {
    return;
  }
  for( int i = 0; i < MAX_NUM_REF; i++ )
  {
    if( scaledRefPic[i] != nullptr )
    {
      scaledRefPic[i]->destroy();
      scaledRefPic[i] = nullptr;
    }
  }
}

bool Slice::checkRPR()
{
  const PPS* pps = getPPS();

  for( int refList = 0; refList < NUM_REF_PIC_LIST_01; refList++ )
  {

    if( refList == 1 && m_eSliceType != B_SLICE )
    {
      continue;
    }

    for( int rIdx = 0; rIdx < m_aiNumRefIdx[refList]; rIdx++ )
    {
      if( m_scaledRefPicList[refList][rIdx]->cs->pcv->lumaWidth != pps->getPicWidthInLumaSamples() || m_scaledRefPicList[refList][rIdx]->cs->pcv->lumaHeight != pps->getPicHeightInLumaSamples() )
      {
        return true;
      }
    }
  }

  return false;
}

#if JVET_Q0117_PARAMETER_SETS_CLEANUP
bool             operator == (const ConstraintInfo& op1, const ConstraintInfo& op2)
{
  if( op1.m_progressiveSourceFlag                        != op2.m_progressiveSourceFlag                          ) return false;
  if( op1.m_interlacedSourceFlag                         != op2.m_interlacedSourceFlag                           ) return false;
  if( op1.m_nonPackedConstraintFlag                      != op2.m_nonPackedConstraintFlag                        ) return false;
  if( op1.m_frameOnlyConstraintFlag                      != op2.m_frameOnlyConstraintFlag                        ) return false;
  if( op1.m_intraOnlyConstraintFlag                      != op2.m_intraOnlyConstraintFlag                        ) return false;
  if( op1.m_maxBitDepthConstraintIdc                     != op2.m_maxBitDepthConstraintIdc                       ) return false;
  if( op1.m_maxChromaFormatConstraintIdc                 != op2.m_maxChromaFormatConstraintIdc                   ) return false;
  if( op1.m_onePictureOnlyConstraintFlag                 != op2.m_onePictureOnlyConstraintFlag                   ) return false;
  if( op1.m_lowerBitRateConstraintFlag                   != op2.m_lowerBitRateConstraintFlag                     ) return false;
  if( op1.m_noQtbttDualTreeIntraConstraintFlag           != op2.m_noQtbttDualTreeIntraConstraintFlag             ) return false;
  if( op1.m_noPartitionConstraintsOverrideConstraintFlag != op2.m_noPartitionConstraintsOverrideConstraintFlag   ) return false;
  if( op1.m_noSaoConstraintFlag                          != op2.m_noSaoConstraintFlag                            ) return false;
  if( op1.m_noAlfConstraintFlag                          != op2.m_noAlfConstraintFlag                            ) return false;
#if JVET_Q0795_CCALF                                                                              
  if( op1.m_noCCAlfConstraintFlag                        != op2.m_noCCAlfConstraintFlag                          ) return false;
#endif                                                                                         
  if( op1.m_noRefWraparoundConstraintFlag                != op2.m_noRefWraparoundConstraintFlag                  ) return false;
  if( op1.m_noTemporalMvpConstraintFlag                  != op2.m_noTemporalMvpConstraintFlag                    ) return false;
  if( op1.m_noSbtmvpConstraintFlag                       != op2.m_noSbtmvpConstraintFlag                         ) return false;
  if( op1.m_noAmvrConstraintFlag                         != op2.m_noAmvrConstraintFlag                           ) return false;
  if( op1.m_noBdofConstraintFlag                         != op2.m_noBdofConstraintFlag                           ) return false;
  if( op1.m_noDmvrConstraintFlag                         != op2.m_noDmvrConstraintFlag                           ) return false;
  if( op1.m_noCclmConstraintFlag                         != op2.m_noCclmConstraintFlag                           ) return false;
  if( op1.m_noMtsConstraintFlag                          != op2.m_noMtsConstraintFlag                            ) return false;
  if( op1.m_noSbtConstraintFlag                          != op2.m_noSbtConstraintFlag                            ) return false;
  if( op1.m_noAffineMotionConstraintFlag                 != op2.m_noAffineMotionConstraintFlag                   ) return false;
  if( op1.m_noBcwConstraintFlag                          != op2.m_noBcwConstraintFlag                            ) return false;
  if( op1.m_noIbcConstraintFlag                          != op2.m_noIbcConstraintFlag                            ) return false;
  if( op1.m_noCiipConstraintFlag                         != op2.m_noCiipConstraintFlag                           ) return false;
  if( op1.m_noFPelMmvdConstraintFlag                     != op2.m_noFPelMmvdConstraintFlag                       ) return false;
#if !JVET_Q0806
  if( op1.m_noTriangleConstraintFlag                     != op2.m_noTriangleConstraintFlag                       ) return false;
#endif
  if( op1.m_noLadfConstraintFlag                         != op2.m_noLadfConstraintFlag                           ) return false;
  if( op1.m_noTransformSkipConstraintFlag                != op2.m_noTransformSkipConstraintFlag                  ) return false;
  if( op1.m_noBDPCMConstraintFlag                        != op2.m_noBDPCMConstraintFlag                          ) return false;
  if( op1.m_noJointCbCrConstraintFlag                    != op2.m_noJointCbCrConstraintFlag                      ) return false;
  if( op1.m_noQpDeltaConstraintFlag                      != op2.m_noQpDeltaConstraintFlag                        ) return false;
  if( op1.m_noDepQuantConstraintFlag                     != op2.m_noDepQuantConstraintFlag                       ) return false;
  if( op1.m_noSignDataHidingConstraintFlag               != op2.m_noSignDataHidingConstraintFlag                 ) return false;
  if( op1.m_noTrailConstraintFlag                        != op2.m_noTrailConstraintFlag                          ) return false;
  if( op1.m_noStsaConstraintFlag                         != op2.m_noStsaConstraintFlag                           ) return false;
  if( op1.m_noRaslConstraintFlag                         != op2.m_noRaslConstraintFlag                           ) return false;
  if( op1.m_noRadlConstraintFlag                         != op2.m_noRadlConstraintFlag                           ) return false;
  if( op1.m_noIdrConstraintFlag                          != op2.m_noIdrConstraintFlag                            ) return false;
  if( op1.m_noCraConstraintFlag                          != op2.m_noCraConstraintFlag                            ) return false;
  if( op1.m_noGdrConstraintFlag                          != op2.m_noGdrConstraintFlag                            ) return false;
  if( op1.m_noApsConstraintFlag                          != op2.m_noApsConstraintFlag                            ) return false;
  return true;
}
bool             operator != (const ConstraintInfo& op1, const ConstraintInfo& op2)
{
  return !(op1 == op2);
}

bool             operator == (const ProfileTierLevel& op1, const ProfileTierLevel& op2)
{
  if (op1.m_tierFlag        != op2.m_tierFlag) return false;
  if (op1.m_profileIdc      != op2.m_profileIdc) return false;
  if (op1.m_numSubProfile   != op2.m_numSubProfile) return false;
  if (op1.m_levelIdc        != op2.m_levelIdc) return false;
  if (op1.m_constraintInfo  != op2.m_constraintInfo) return false;
  if (op1.m_subProfileIdc   != op2.m_subProfileIdc) return false;

  for (int i = 0; i < MAX_TLAYER - 1; i++)
  {
    if (op1.m_subLayerLevelPresentFlag[i] != op2.m_subLayerLevelPresentFlag[i])
    {
      return false;
    }
  }
  for (int i = 0; i < MAX_TLAYER; i++)
  {
    if (op1.m_subLayerLevelIdc[i] != op2.m_subLayerLevelIdc[i])
    {
      return false;
    }
  }
  return true;
}
bool             operator != (const ProfileTierLevel& op1, const ProfileTierLevel& op2)
{
  return !(op1 == op2);
}
#endif

#if ENABLE_TRACING
void xTraceVPSHeader()
{
  DTRACE( g_trace_ctx, D_HEADER, "=========== Video Parameter Set     ===========\n" );
}

#if !JVET_Q0117_PARAMETER_SETS_CLEANUP
void xTraceDPSHeader()
{
  DTRACE( g_trace_ctx, D_HEADER, "=========== Decoding Parameter Set     ===========\n" );
}
#else
void xTraceDCIHeader()
{
  DTRACE( g_trace_ctx, D_HEADER, "=========== Decoding Capability Information     ===========\n" );
}
#endif

void xTraceSPSHeader()
{
  DTRACE( g_trace_ctx, D_HEADER, "=========== Sequence Parameter Set  ===========\n" );
}

void xTracePPSHeader()
{
  DTRACE( g_trace_ctx, D_HEADER, "=========== Picture Parameter Set  ===========\n" );
}

void xTraceAPSHeader()
{
  DTRACE(g_trace_ctx, D_HEADER, "=========== Adaptation Parameter Set  ===========\n");
}

void xTracePictureHeader()
{
  DTRACE( g_trace_ctx, D_HEADER, "=========== Picture Header ===========\n" );
}

void xTraceSliceHeader()
{
  DTRACE( g_trace_ctx, D_HEADER, "=========== Slice ===========\n" );
}

void xTraceAccessUnitDelimiter()
{
  DTRACE( g_trace_ctx, D_HEADER, "=========== Access Unit Delimiter ===========\n" );
}
#endif