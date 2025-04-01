define dso_local i32 @foo(i32 noundef %0) #0 {
  %2 = sub nsw i32 %0, 2
  %3 = add nsw i32 %2, 1
  %4 = add nsw i32 %3, 2
  %5 = sub nsw i32 %4, 1
  %6 = add nsw i32 %5, 10
  %7 = mul nsw i32 %6, 2
  ret i32 %7
}
