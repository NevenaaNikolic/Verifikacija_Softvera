int funkcija_1 (int a)
{
	int c = 3;
	
	return a + c;
}

int main ()
{
   int a = 10;
   int b = a - a;
   int d = funkcija_1 (a);
   
   return (b + d);
}
