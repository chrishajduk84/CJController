#include <stdlib.h>
#include <stdio.h>
#include <string>
#include <iostream>
#include <list>

static string get_driver(const string& tty);
static void register_comport( list<string>& comList, list<string>& comList8250, const string& dir);
static void probe_serial8250_comports(list<string>& comList, list<string> comList8250);
list<string> getComList();
