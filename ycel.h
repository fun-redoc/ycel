#ifndef __YCEL_H__
#define __YCEL_H__

typedef enum {
    CSV = 1,
    PRETTY = 2,
    DUMMY = 4,
} ERunState;

typedef struct {
    ERunState run_state;
    const char *param_name;
} TRunStateParamMap;

TRunStateParamMap run_state_param_map[] = {
    {CSV, "-csv"},
    {PRETTY, "-pretty"},
    {DUMMY, "-dummy"},
};
ERunState get_runstate_from_arg(const char *s);
const char *get_arg_from_runstate(ERunState rs);
int mutually_exclusive_run_states[] =
{
    CSV | PRETTY
};
int run_state = 0;

#endif