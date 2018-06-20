{ ****************************************************************************** }
{ * h264Motion_est.pas        by qq600585                                      * }
{ * https://github.com/PassByYou888/CoreCipher                                 * }
{ * https://github.com/PassByYou888/ZServer4D                                  * }
{ * https://github.com/PassByYou888/zExpression                                * }
{ * https://github.com/PassByYou888/zTranslate                                 * }
{ * https://github.com/PassByYou888/zSound                                     * }
{ * https://github.com/PassByYou888/zAnalysis                                  * }
{ ****************************************************************************** }

unit h264Motion_est;

{$I zDefine.inc}
{$POINTERMATH ON}

interface

uses
  h264Stdint, h264common, h264util, h264motion_comp, h264inter_pred, h264motion_est_search, CoreClasses;

type
  TScoreListItem = packed record
    score: int32_t;
    mv: motionvec_t;
    refidx: int32_t;
  end;

  { TMotionEstimator }

  TMotionEstimator = class
  private
    width, height: int32_t;
    mb_width, mb_height: int32_t;
    mv_field: motionvec_p;
    predicted_mv_list: TMotionVectorList;
    ref_count: int32_t;
    _subme: int32_t;

    scoreList: array of TScoreListItem;
    SearchRegion: TRegionSearch;
    MotionCompensator: TMotionCompensation;
    InterCost: IInterPredCostEvaluator;

    procedure EstimateMultiRef(var mb: macroblock_t; var fenc: frame_t);
    procedure EstimateSingleRef(var mb: macroblock_t; var fenc: frame_t);
    procedure LoadMVPredictors(const mbx, mby: int32_t);
    class function ClipMVRange(const mv: motionvec_t; range: int32_t): motionvec_t; inline;
    procedure SetNumReferences(AValue: int32_t);
    procedure SetSubMELevel(AValue: int32_t);

  public
    property Subme: int32_t write SetSubMELevel;

    property NumReferences: int32_t read ref_count write SetNumReferences;
    constructor Create(const w, h, mbw, mbh: int32_t; mc: TMotionCompensation; inter_cost: IInterPredCostEvaluator);
    destructor Destroy; override;
    procedure Estimate(var mb: macroblock_t; var fenc: frame_t);
  end;

  (* ******************************************************************************
    ****************************************************************************** *)
implementation

procedure swapItem(var a, b: TScoreListItem); inline;
var
  t: TScoreListItem;
begin
  t := a;
  a := b;
  b := t;
end;

{ TMotionEstimator }

procedure TMotionEstimator.LoadMVPredictors(const mbx, mby: int32_t);
var
  mv_a, mv_b: motionvec_t;
begin
  // A, B, avg
  if mbx > 0 then
    begin
      mv_a := mv_field[mby * mb_width + mbx - 1];
      predicted_mv_list.Add(mv_a);
    end
  else
      mv_a := ZERO_MV;
  if mby > 0 then
    begin
      mv_b := mv_field[(mby - 1) * mb_width + mbx];
      predicted_mv_list.Add(mv_b);
    end
  else
      mv_b := ZERO_MV;
  if not(mv_a = mv_b) then
      predicted_mv_list.Add((mv_a + mv_b) / 2);

  // C, D
  if (mby > 0) and (mbx < mb_width - 1) then
      predicted_mv_list.Add(mv_field[(mby - 1) * mb_width + mbx + 1]);
  if (mby > 0) and (mbx > 0) then
      predicted_mv_list.Add(mv_field[(mby - 1) * mb_width + mbx - 1]);

  // last frame: same position, posx+1
  predicted_mv_list.Add(mv_field[mby * mb_width + mbx]);
  if mbx < mb_width - 2 then
      predicted_mv_list.Add(mv_field[mby * mb_width + mbx + 1]);
end;

class function TMotionEstimator.ClipMVRange(const mv: motionvec_t; range: int32_t): motionvec_t;
begin
  result := XYToMVec(clip3(-range, mv.x, range), clip3(-range, mv.y, range));
end;

procedure TMotionEstimator.SetNumReferences(AValue: int32_t);
begin
  if ref_count = AValue then
      Exit;
  ref_count := AValue;
  SetLength(scoreList, ref_count);
end;

procedure TMotionEstimator.SetSubMELevel(AValue: int32_t);
begin
  _subme := AValue;
end;

constructor TMotionEstimator.Create(const w, h, mbw, mbh: int32_t; mc: TMotionCompensation; inter_cost: IInterPredCostEvaluator);
var
  size: int32_t;
begin
  inherited Create;
  width := w;
  height := h;
  mb_width := mbw;
  mb_height := mbh;
  size := mbw * mbh * sizeof(motionvec_t);
  mv_field := getMemory(size);
  fillPtrByte(mv_field, size, 0);
  SetNumReferences(1);

  predicted_mv_list := TMotionVectorList.Create;

  InterCost := inter_cost;
  MotionCompensator := mc;

  SearchRegion := TRegionSearch.Create(width, height, mc, InterCost);
end;

destructor TMotionEstimator.Destroy;
begin
  freemem(mv_field);
  scoreList := nil;
  predicted_mv_list.Free;
  SearchRegion.Free;
  inherited Destroy;
end;

procedure TMotionEstimator.Estimate(var mb: macroblock_t; var fenc: frame_t);
begin
  SearchRegion.cur := mb.pixels;
  SearchRegion._mbx := mb.x * 16;
  SearchRegion._mby := mb.y * 16;
  InterCost.SetQP(mb.qp);

  predicted_mv_list.Clear;
  predicted_mv_list.Add(mb.mvp);
  LoadMVPredictors(mb.x, mb.y);

  if NumReferences = 1 then
      EstimateSingleRef(mb, fenc)
  else
      EstimateMultiRef(mb, fenc);
end;

procedure TMotionEstimator.EstimateSingleRef(var mb: macroblock_t; var fenc: frame_t);
var
  fref: frame_p;
begin
  mb.fref := fenc.refs[0];
  fref := mb.fref;
  InterCost.SetMVPredAndRefIdx(mb.mvp, 0);

  SearchRegion.PickFPelStartingPoint(fref, predicted_mv_list);
  mb.mv := SearchRegion.SearchFPel(mb, fref);

  if _subme > 0 then
      mb.mv := SearchRegion.SearchHPel(mb, fref);
  if _subme > 1 then
      mb.mv := SearchRegion.SearchQPel(mb, fref, _subme > 2, _subme > 3);

  mb.mv := ClipMVRange(mb.mv, 512);
  MotionCompensator.Compensate(fref, mb.mv, mb.x, mb.y, mb.mcomp);
  mv_field[mb.y * mb_width + mb.x] := mb.mv;
end;

procedure TMotionEstimator.EstimateMultiRef(var mb: macroblock_t; var fenc: frame_t);

// lowest score to lowest index
  procedure SortList;
  var
    i, j: int32_t;
  begin
    for i := 0 to Length(scoreList) - 1 do
      for j := 0 to Length(scoreList) - 2 do
        if scoreList[j].score > scoreList[j + 1].score then
            swapItem(scoreList[j], scoreList[j + 1]);
  end;

  function CountLower(const cutoff_score: int32_t): int32_t;
  begin
    result := 0;
    while scoreList[result].score < cutoff_score do
      begin
        inc(result);
        if result = Length(scoreList) then
            break;
      end;
  end;

var
  i: int32_t;
  score: int32_t;
  mv: motionvec_t;
  best_refidx, best_score: int32_t;
  fref: frame_p;
  tested_ref_count: int32_t;

begin
  mb.fref := fenc.refs[0];
  fref := mb.fref;

  // fpel test
  for i := 0 to ref_count - 1 do
    begin
      fref := fenc.refs[i];
      SearchRegion.PickFPelStartingPoint(fref, predicted_mv_list);
      scoreList[i].mv := SearchRegion.SearchFPel(mb, fref);
      scoreList[i].score := SearchRegion.LastSearchScore;
      scoreList[i].refidx := i;
    end;

  // cut off refs with far worse score than best
  SortList;
  tested_ref_count := CountLower(scoreList[0].score * 2);

  // hpel/qpel
  best_score := MaxInt;
  best_refidx := scoreList[0].refidx;
  mv := scoreList[0].mv;
  for i := 0 to tested_ref_count - 1 do
    begin
      mb.mv := scoreList[i].mv;
      mb.ref := scoreList[i].refidx;
      fref := fenc.refs[mb.ref];

      InterCost.SetMVPredAndRefIdx(mb.mvp, mb.ref);
      mb.mv := SearchRegion.SearchHPel(mb, fref);
      mb.mv := SearchRegion.SearchQPel(mb, fref, _subme > 2, _subme > 3);

      score := SearchRegion.LastSearchScore;
      if score < best_score then
        begin
          mv := mb.mv;
          best_refidx := mb.ref;
          best_score := score;
        end;
    end;

  // restore best
  fref := fenc.refs[best_refidx];
  mb.ref := best_refidx;
  mb.fref := fref;
  mb_load_mvs(mb, fenc, ref_count);

  mb.mv := ClipMVRange(mv, 512);
  MotionCompensator.Compensate(fref, mb.mv, mb.x, mb.y, mb.mcomp);
  mv_field[mb.y * mb_width + mb.x] := mb.mv;
end;

end.