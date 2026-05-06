#pragma once

typedef struct {
	char name[30];
	char adres[70];
	char phones[50];
	char special[50];
	char form[30];
	int opens[7];
	int closes[7];
} magaz;

typedef struct {
	int len;
	magaz** base;
}box;


box getbase(char *name);

box findstores(box *base);

void printer(box *base);

void delit(box *base);