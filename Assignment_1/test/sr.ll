define dso_local i32 @foo(i32 noundef %0, i32 noundef %1) #0 {

  %3 = mul nsw i32 %0, 16
  %4 = mul nsw i32 %0, 15
  %5 = mul nsw i32 %0, 17
  %6 = mul nsw i32 %0, 18

  %7 = sdiv i32 %1, 8
  %8 = udiv i32 %1, 8

  %9 = mul nsw i32 %3, %4
  %10 = mul nsw i32 %5, %6

  %11 = mul nsw i32 %8, %9
  ret i32 %11
}
