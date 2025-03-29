define dso_local i32 @foo(i32 noundef %0, i32 noundef %1) #0 {
  %3 = add nsw i32 %0, 0
  %4 = mul nsw i32 %1, 1

  %5 = add nsw i32 %3, 1
  %6 = mul nsw i32 %4, 2

  %7 = mul nsw i32 %5, %6
  ret i32 %7
}
