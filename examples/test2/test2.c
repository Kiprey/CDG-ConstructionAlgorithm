int main()
{
	int *p, *q;
	int a, b, c,d=2,e;
	if (c) {
		p = &a;
		q = &b;
		d=1;
	}
	else {
		p = &b;
		q = &c;
		d=0;
	}
	switch(d){
	case 0:
		e=10;
		break;
	case 1:
		e=2;
		break;
	default:
		e=3;
	}
	return 0;
}
