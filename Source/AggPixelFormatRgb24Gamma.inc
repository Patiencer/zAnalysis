//
// AggPas 2.4 RM3 pixel format definition file
//

procedure BlendPixRgbGamma(Gamma: TAggGamma; P: PInt8u;
  Cr, Cg, Cb, Alpha: Integer; Cover: Cardinal = 0);
var
  R, G, B: Cardinal;
begin
  R := Gamma.Dir[TAggOrderRGB(Pointer(P)^).R];
  G := Gamma.Dir[TAggOrderRGB(Pointer(P)^).G];
  B := Gamma.Dir[TAggOrderRGB(Pointer(P)^).B];

  TAggOrderRGB(Pointer(P)^).R :=
    Int8u(Integer(Gamma.Inv[(((Integer(Gamma.Dir[Cr]) - R) * Alpha)
    shr CAggBaseShift) + R]));
  TAggOrderRGB(Pointer(P)^).G :=
    Int8u(Integer(Gamma.Inv[(((Integer(Gamma.Dir[Cg]) - G) * Alpha)
    shr CAggBaseShift) + G]));
  TAggOrderRGB(Pointer(P)^).B :=
    Int8u(Integer(Gamma.Inv[(((Integer(Gamma.Dir[Cb]) - B) * Alpha)
    shr CAggBaseShift) + B]));
end;

procedure CopyOrBlendPixRgbGamma(Gamma: TAggGamma; P: PInt8u;
  C: PAggColor; Cover: Cardinal); overload;
var
  Alpha: Cardinal;
begin
  if C.Rgba8.A <> 0 then
  begin
    Alpha := (C.Rgba8.A * (Cover + 1)) shr 8;

    if Alpha = CAggBaseMask then
    begin
      TAggOrderRGB(Pointer(P)^).R := C.Rgba8.R;
      TAggOrderRGB(Pointer(P)^).G := C.Rgba8.G;
      TAggOrderRGB(Pointer(P)^).B := C.Rgba8.B;

    end
    else
      BlendPixRgbGamma(Gamma, P, C.Rgba8.R, C.Rgba8.G, C.Rgba8.B, Alpha, Cover);
  end;
end;

procedure CopyOrBlendPixRgbGamma(Gamma: TAggGamma; P: PInt8u;
  C: PAggColor); overload;
begin
  if C.Rgba8.A <> 0 then
    if C.Rgba8.A = CAggBaseMask then
    begin
      TAggOrderRGB(Pointer(P)^).R := C.Rgba8.R;
      TAggOrderRGB(Pointer(P)^).G := C.Rgba8.G;
      TAggOrderRGB(Pointer(P)^).B := C.Rgba8.B;
    end
    else
      BlendPixRgbGamma(Gamma, P, C.Rgba8.R, C.Rgba8.G, C.Rgba8.B, C.Rgba8.A);
end;

procedure Rgb24GammaBlendPixel(This: TAggPixelFormatProcessor; X, Y: Integer;
  C: PAggColor; Cover: Int8u);
begin
  CopyOrBlendPixRgbGamma(This.Gamma,
    PInt8u(PtrComp(This.RenderingBuffer.Row(Y)) + X + X + X), C, Cover);
end;

procedure Rgb24GammaBlendHorizontalLine(This: TAggPixelFormatProcessor;
  X, Y: Integer; Len: Cardinal; C: PAggColor; Cover: Int8u);
var
  P: PInt8u;
  Alpha: Cardinal;
begin
  if C.Rgba8.A <> 0 then
  begin
    P := PInt8u(PtrComp(This.RenderingBuffer.Row(Y)) + X + X + X);

    Alpha := (C.Rgba8.A * (Cover + 1)) shr 8;

    if Alpha = CAggBaseMask then
      repeat
        TAggOrderRGB(Pointer(P)^).R := C.Rgba8.R;
        TAggOrderRGB(Pointer(P)^).G := C.Rgba8.G;
        TAggOrderRGB(Pointer(P)^).B := C.Rgba8.B;

        Inc(PtrComp(P), 3);
        Dec(Len);
      until Len = 0
    else
      repeat
        BlendPixRgbGamma(This.Gamma, P, C.Rgba8.R, C.Rgba8.G, C.Rgba8.B, Alpha);

        Inc(PtrComp(P), 3);
        Dec(Len);
      until Len = 0;
  end;
end;

procedure Rgb24GammaBlendVerticalLine(This: TAggPixelFormatProcessor;
  X, Y: Integer; Len: Cardinal; C: PAggColor; Cover: Int8u);
var
  P: PInt8u;
  Alpha: Cardinal;
begin
  if C.Rgba8.A <> 0 then
  begin
    P := PInt8u(PtrComp(This.RenderingBuffer.Row(Y)) + X + X + X);

    Alpha := (C.Rgba8.A * (Cover + 1)) shr 8;

    if Alpha = CAggBaseMask then
      repeat
        TAggOrderRGB(Pointer(P)^).R := C.Rgba8.R;
        TAggOrderRGB(Pointer(P)^).G := C.Rgba8.G;
        TAggOrderRGB(Pointer(P)^).B := C.Rgba8.B;

        P := PInt8u(This.RenderingBuffer.NextRow(P));

        Dec(Len);
      until Len = 0
    else
      repeat
        BlendPixRgbGamma(This.Gamma, P, C.Rgba8.R, C.Rgba8.G, C.Rgba8.B, Alpha,
          Cover);

        P := PInt8u(This.RenderingBuffer.NextRow(P));

        Dec(Len);
      until Len = 0;
  end;
end;

procedure Rgb24GammaBlendSolidHorizontalSpan(This: TAggPixelFormatProcessor;
  X, Y: Integer; Len: Cardinal; C: PAggColor; Covers: PInt8u);
var
  P: PInt8u;
  Alpha: Cardinal;
begin
  if C.Rgba8.A <> 0 then
  begin
    P := PInt8u(PtrComp(This.RenderingBuffer.Row(Y)) + X + X + X);

    repeat
      Alpha := (C.Rgba8.A * (Covers^ + 1)) shr 8;

      if Alpha = CAggBaseMask then
      begin
        TAggOrderRGB(Pointer(P)^).R := C.Rgba8.R;
        TAggOrderRGB(Pointer(P)^).G := C.Rgba8.G;
        TAggOrderRGB(Pointer(P)^).B := C.Rgba8.B;
      end
      else
        BlendPixRgbGamma(This.Gamma, P, C.Rgba8.R, C.Rgba8.G, C.Rgba8.B, Alpha,
          Covers^);

      Inc(PtrComp(P), 3);
      Inc(PtrComp(Covers));
      Dec(Len);
    until Len = 0;
  end;
end;

procedure Rgb24GammaBlendSolidVerticalSpan(This: TAggPixelFormatProcessor;
  X, Y: Integer; Len: Cardinal; C: PAggColor; Covers: PInt8u);
var
  P: PInt8u;
  Alpha: Cardinal;
begin
  if C.Rgba8.A <> 0 then
  begin
    P := PInt8u(PtrComp(This.RenderingBuffer.Row(Y)) + X + X + X);

    repeat
      Alpha := (C.Rgba8.A * (Covers^ + 1)) shr 8;

      if Alpha = CAggBaseMask then
      begin
        TAggOrderRGB(Pointer(P)^).R := C.Rgba8.R;
        TAggOrderRGB(Pointer(P)^).G := C.Rgba8.G;
        TAggOrderRGB(Pointer(P)^).B := C.Rgba8.B;
      end
      else
        BlendPixRgbGamma(This.Gamma, P, C.Rgba8.R, C.Rgba8.G, C.Rgba8.B, Alpha,
          Covers^);

      P := PInt8u(This.RenderingBuffer.NextRow(P));

      Inc(PtrComp(Covers));
      Dec(Len);
    until Len = 0;
  end;
end;

procedure Rgb24GammaBlendColorHorizontalSpan(This: TAggPixelFormatProcessor; X,
  Y: Integer; Len: Cardinal; Colors: PAggColor; Covers: PInt8u; Cover: Int8u);
var
  P: PInt8u;
begin
  P := PInt8u(PtrComp(This.RenderingBuffer.Row(Y)) + X + X + X);

  if Covers <> nil then
    repeat
      CopyOrBlendPixRgbGamma(This.Gamma, P, Colors, Covers^);

      Inc(PtrComp(Colors), SizeOf(TAggColor));
      Inc(PtrComp(Covers), SizeOf(Int8u));
      Inc(PtrComp(P), 3);
      Dec(Len);
    until Len = 0
  else if Cover = 255 then
    repeat
      CopyOrBlendPixRgbGamma(This.Gamma, P, Colors);

      Inc(PtrComp(Colors), SizeOf(TAggColor));
      Inc(PtrComp(P), 3);
      Dec(Len);
    until Len = 0
  else
    repeat
      CopyOrBlendPixRgbGamma(This.Gamma, P, Colors, Cover);

      Inc(PtrComp(Colors), SizeOf(TAggColor));
      Inc(PtrComp(P), 3);
      Dec(Len);
    until Len = 0;
end;

procedure Rgb24GammaBlendColorVerticalSpan(This: TAggPixelFormatProcessor; X,
  Y: Integer; Len: Cardinal; Colors: PAggColor; Covers: PInt8u; Cover: Int8u);
var
  P: PInt8u;
begin
  P := PInt8u(PtrComp(This.RenderingBuffer.Row(Y)) + X + X + X);

  if Covers <> nil then
    repeat
      CopyOrBlendPixRgbGamma(This.Gamma, P, Colors, Covers^);

      Inc(PtrComp(Colors), SizeOf(TAggColor));
      Inc(PtrComp(Covers), SizeOf(Int8u));

      P := PInt8u(This.RenderingBuffer.NextRow(P));

      Dec(Len);
    until Len = 0
  else if Cover = 255 then
    repeat
      CopyOrBlendPixRgbGamma(This.Gamma, P, Colors);

      Inc(PtrComp(Colors), SizeOf(TAggColor));

      P := PInt8u(This.RenderingBuffer.NextRow(P));

      Dec(Len);
    until Len = 0
  else
    repeat
      CopyOrBlendPixRgbGamma(This.Gamma, P, Colors, Cover);

      Inc(PtrComp(Colors), SizeOf(TAggColor));

      P := PInt8u(This.RenderingBuffer.NextRow(P));

      Dec(Len);
    until Len = 0;
end;

procedure Rgb24GammaBlendFrom(This, From: TAggPixelFormatProcessor;
  SourcePtr: PInt8u; Xdst, Ydst, Xsrc, Ysrc: Integer; Len: Cardinal;
  Cover: Int8u);
var
  Psrc, Pdst: PInt8u;

  Color: TAggColor;
  Alpha: Cardinal;
begin
  Psrc := SourcePtr;
  Pdst := PInt8u(PtrComp(This.RenderingBuffer.Row(Ydst)) + 3 * Xdst *
    SizeOf(Int8u));

  if Cover = 255 then
    repeat
      Alpha := PInt8u(PtrComp(Psrc) + From.Order.A)^;

      if Alpha <> 0 then
        if Alpha = CAggBaseMask then
        begin
          TAggOrderRGB(Pointer(Pdst)^).R :=
            PInt8u(PtrComp(Psrc) + From.Order.R)^;
          TAggOrderRGB(Pointer(Pdst)^).G :=
            PInt8u(PtrComp(Psrc) + From.Order.G)^;
          TAggOrderRGB(Pointer(Pdst)^).B :=
            PInt8u(PtrComp(Psrc) + From.Order.B)^;

        end
        else
          BlendPixRgbGamma(This.Gamma, Pdst,
            PInt8u(PtrComp(Psrc) + From.Order.R)^,
            PInt8u(PtrComp(Psrc) + From.Order.G)^,
            PInt8u(PtrComp(Psrc) + From.Order.B)^, Alpha);

      Inc(PtrComp(Psrc), 4);
      Inc(PtrComp(Pdst), 3);
      Dec(Len);
    until Len = 0
  else
    repeat
      Color.FromRgbaInteger(PInt8u(PtrComp(Psrc) + From.Order.R)^,
        PInt8u(PtrComp(Psrc) + From.Order.G)^,
        PInt8u(PtrComp(Psrc) + From.Order.B)^,
        PInt8u(PtrComp(Psrc) + From.Order.A)^);

      CopyOrBlendPixRgbGamma(This.Gamma, Pdst, @Color, Cover);

      Inc(PtrComp(Psrc), 4);
      Inc(PtrComp(Pdst), 3);
      Dec(Len);
    until Len = 0;
end;

procedure Rgb24GammaBlendFromColor(This, From: TAggPixelFormatProcessor;
  Color: PAggColor; Xdst, Ydst, Xsrc, Ysrc: Integer; Len: Cardinal;
  Cover: Int8u);
var
  Ppsz: Cardinal;
  Psrc, Pdst: PInt8u;
begin
  Ppsz := From.PixWidth;
  Psrc := From.GetRowPointer(Ysrc);

  if Psrc <> nil then
  begin
    Pdst := PInt8u(PtrComp(This.RenderingBuffer.RowXY(Xdst, Ydst, Len)) +
      3 * Xdst * SizeOf(Int8u));

    repeat
      CopyOrBlendPixRgbGamma(This.Gamma, Pdst, Color,
        ShrInt32(Psrc^ * Cover + CAggBaseMask, CAggBaseShift));

      Inc(PtrComp(Psrc), Ppsz);
      Inc(PtrComp(Pdst), 3);
      Dec(Len);
    until Len = 0;
  end;
end;

procedure Rgb24GammaBlendFromLUT(This, From: TAggPixelFormatProcessor;
  ColorLUT: PAggColor; Xdst, Ydst, Xsrc, Ysrc: Integer; Len: Cardinal;
  Cover: Int8u);
var
  Ppsz: Cardinal;
  Psrc, Pdst: PInt8u;
  Color: PAggColor;
begin
  Ppsz := From.PixWidth;
  Psrc := From.GetRowPointer(Ysrc);

  if Psrc <> nil then
  begin
    Pdst := PInt8u(PtrComp(This.RenderingBuffer.RowXY(Xdst, Ydst, Len)) +
      3 * Xdst * SizeOf(Int8u));

    if Cover = 255 then
      repeat
        Color := PAggColor(PtrComp(ColorLUT) + Psrc^ * SizeOf(TAggColor));

        BlendPixRgbGamma(This.Gamma, Pdst, Color.Rgba8.R, Color.Rgba8.G,
          Color.Rgba8.B, Color.Rgba8.A);

        Inc(PtrComp(Psrc), Ppsz);
        Inc(PtrComp(Pdst), 3);
        Dec(Len);
      until Len = 0
    else
      repeat
        CopyOrBlendPixRgbGamma(This.Gamma, Pdst,
          PAggColor(PtrComp(ColorLUT) + Psrc^ * SizeOf(TAggColor)), Cover);

        Inc(PtrComp(Psrc), Ppsz);
        Inc(PtrComp(Pdst), 3);
        Dec(Len);
      until Len = 0;
  end;
end;
