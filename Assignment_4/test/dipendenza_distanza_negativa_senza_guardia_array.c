int foo() {
 int x[20],y[20],w=0;
 for (int i=0;i<10;i++) {
  x[i]=i;
 }
 for (int i=0;i<10;i++) {
  y[i]=x[i+1];
 }
 return 0;
}
