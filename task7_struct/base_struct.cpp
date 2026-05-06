#include "base_struct.hpp"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void generate(box* base) {
	int i = 0;
	base->base = (magaz**)malloc(base->len * sizeof(magaz*));
	if (base->base == NULL) {
		printf("MEMORY ERROR 1");
		base->len = 0;
		free(base->base);
	}

	for (i = 0; i < base->len; i++) {
		base->base[i] = (magaz*)malloc(sizeof(magaz));
		if (base->base[i] == NULL) {
			printf("MEMORY ERROR 2 [%d]", i);
			for (int j = 0; j < i; j++) {
				free(base->base[j]);
			}
			free(base->base);
			base->len = 0;
		}
	}
}

box getbase(char *filename) {
	//обьявления
	box ans;
	FILE* file = fopen(filename, "r");
	int i;
	char c[1000];
	ans.len = 0;
	//код

	if (file == NULL) {
		printf("FILE ERROR");
		return ans;
	}
	while (fgets(c, 1000, file) != NULL) {
		ans.len++;
	}
	rewind(file);

	generate(&ans);
	if (ans.len == 0)return ans;

	for (i = 0; i < ans.len; i++) {
		char *tmp, *mpt;
		char* token;
		int u;
		int topens[7], tcloses[7];
		if (fgets(c, 1000, file) == NULL) {
			printf("READ ERROR [%d]", i);
			ans.len = 0;
			return ans;
		}
		tmp = strtok(c, ":");
		strcpy(ans.base[i]->name, tmp);
		tmp = strtok(NULL, ":");
		strcpy(ans.base[i]->adres, tmp);
		tmp = strtok(NULL, ":");
		strcpy(ans.base[i]->phones, tmp);
		tmp = strtok(NULL, ":");
		strcpy(ans.base[i]->special, tmp);
		tmp = strtok(NULL, ":");
		strcpy(ans.base[i]->form, tmp);

		tmp = strtok(NULL, ":");

		for (u = 0; u < 7; u++) {
			int oh = 0, om = 0, kh = 0, km = 0;
			char* itok;
			if (strcmp(tmp, "c") == 0) {
				topens[u] = -1;
				tcloses[u] = -1;
			}
			else {
				mpt = strtok_s(tmp, ",", &token);
				sscanf(mpt, "%d.%d-%d.%d", &oh, &om, &kh, &km);
				topens[u] = oh * 60 + om;
				tcloses[u] = kh * 60 + km;
			}
		}
		memcpy(ans.base[i]->opens, topens, sizeof(topens));
		memcpy(ans.base[i]->closes, tcloses, sizeof(tcloses));
	}
	fclose(file);
	return ans;
}

int timesum(int* opens, int* closes) {
	int i, sum = 0;
	for (i = 0; i < 7; i++) {
		sum += closes[i] - opens[i];
	}
	return sum;
}

box findstores(box *base) {
	int i;
	box ans;
	ans.len = 0;
	int* tmp = (int*)malloc(sizeof(int) * base->len);
	for (i = 0; i < base->len; i++) {
		if (strcmp(base->base[i]->special, "food") == 0 && timesum(base->base[i]->opens, base->base[i]->closes) == 10080) {
			tmp[ans.len] = i;
			ans.len++;
		}
	}
	generate(&ans);
	for (i = 0; i < ans.len; i++) {
		*(ans.base[i]) = *(base->base[tmp[i]]);
	}
	free(tmp);
	return ans;
}

void printer(box *base) {
	int i;
	if (base->len == 0) {
		printf("no stores found");
	}
	else {
		for (i = 0; i < base->len; i++) {
			printf("%s \n", base->base[i]->name);
			printf("	adress: %s \n", base->base[i]->adres);
			printf("	phones: %s \n", base->base[i]->phones);
			printf("	form:   %s \n", base->base[i]->form);
		}
		printf(" \ntotal stores founded: %d", base->len);
	}
}

void delit(box* base) {
	int i;
	for (i = 0; i < base->len; i++) {
		free(base->base[i]);
	}
	free(base->base);
}
