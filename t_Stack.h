#include <stdio.h>
#include <stdlib.h>
typedef struct trans
{
	int position;
	double timeAlive;
	double lifeTime;
	double strength;
	double exponent;
}t_trans;

typedef struct _tag_stack_
{
	t_trans a[10000];
	int top;
}Sqstack;


Sqstack* t_InitStack()
{
	Sqstack* ret = NULL;
	ret = (Sqstack*)malloc(sizeof(Sqstack));
	if (ret)
	{
		ret->top = 0;
	}
	return ret;
}

int t_Push(Sqstack* stack, t_trans data)
{
	stack->a[stack->top] = data;
	stack->top++;
	return 1;
}

void t_splice(Sqstack* stack, int k, int l)
{
	for (int i = k; i < stack->top - l; i++)
	{
		stack->a[i] = stack->a[i + l];
	}
	stack->top -= l;
}


void t_Play(Sqstack* stack)
{
	int i = 0;
	if (stack->top == 0)
	{
		printf("It is empty\n");
	}
	else
	{
		for (i = 0; i < stack->top; i++)
		{
			printf("data%f\n", stack->a[i]);
		}
	}
}

int t_Pop(Sqstack* stack, t_trans* data)
{
	if (stack->top == 0)
	{
		printf("the stack is empty\n");
	}
	else
	{
		stack->top--;
		*data = stack->a[stack->top];
	}
	return 1;
}


//int main()
//{
//	Sqstack* qq;
//
//	qq = InitStack();
//
//	printf("length1:%d\n", qq->top);
//	for (int i = 0; i < 5; i++)
//	{
//		double j = i + 0.005;
//		Push(qq, j);
//	}
//	Play(qq);
//	printf("length2:%d\n", qq->top);
//	double u;
//	printf("/n3:%f\n", qq->a[3]);
//
//	Pop(qq, &u);
//	printf("pop%f\n", u);
//	Pop(qq, &u);
//	printf("pop%f\n", u);
//	Pop(qq, &u);
//	printf("pop%f\n", u);
//	Pop(qq, &u);
//	
//	printf("pop%f\n", u);
//	Pop(qq, &u);
//	printf("pop%f\n", u);
//	Pop(qq, &u);
//	printf("%f\n", u);
//
//	return 1;
//}