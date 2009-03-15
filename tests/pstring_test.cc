#include "testlib.h"
#include "workq/pstring.h"

void test_assignment_operator()
{
  WQ::PString st;

  st = "hello";

  check( st == "hello" );
}

void test_append_operator()
{
  WQ::PString st("hello");
  st += " world";
  check( st == "hello world" );
}

int main()
{
  run_tests("PString:", test_assignment_operator, test_append_operator, NULL);
  return 0;
}
