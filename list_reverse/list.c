#include <stdio.h>
#include <stdlib.h>

struct s {
	int a;
	struct s *next;
};


int main()
{
	struct s *head = NULL;
	int i = 0;

	for(i = 0; i < 8; i++){
		struct s *n = (struct s*)malloc(sizeof(struct s));
		struct s *tmp = NULL;
		
		tmp = head;

		n->a = i;
		n->next = tmp;
		head = n;

	}

	struct s *tt = head;
	while(tt){
		printf("%d-\n", tt->a);
		tt = tt->next;
	}

	struct s *oo = head;
	struct s *list1 = NULL;
	struct s *list2 = NULL;
	i = 0;
	while(oo){
		 if(i == 0){
			list1 = oo;
			list2 = oo->next;
			oo = oo->next;
			list1->next = NULL;
		 }else{
			struct s *tmp = NULL;
			tmp = list1;
			list1 = oo;
			list2 = oo->next;
			oo = oo->next;
			list1->next = tmp;
		 }
		 i++;
	}

	struct s *ttt = list1;
	while(ttt){
		printf("%d\n", ttt->a);
		ttt = ttt->next;
	}

	return 0;
}

