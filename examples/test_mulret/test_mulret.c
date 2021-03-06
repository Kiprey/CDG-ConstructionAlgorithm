/*
 * Alias due to lack of path-sensitivity.
 * Author: Sen Ye
 * Date: 06/09/2013
 */
int main()
{
	int *p, *q;
	int a, b, c,d=2,e;
	if (c) {
		p = &a;
		q = &b;
		d=1;return 0;
	}
	else {
		p = &b;
		q = &c;
		d=0;
	}
	switch(d){
	case 0:
		e=10;
		return 0;
	case 1:
		e=2;return 1;
	case 2:
		e=2;return 2;
	default:
		e=3;return 3;
	}
	return 4;
}
