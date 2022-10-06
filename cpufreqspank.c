#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <slurm/spank.h>
#include <sys/wait.h>

SPANK_PLUGIN(cpufreqspank, 1)

#define FREQ_ENV_VAR "SLURM_CPU_FREQ"
#define FREQ_NOT_SET -1.0
static float freq = FREQ_NOT_SET;
static int _cpufreq_opt_process(int val, const char *optarg, int remote);
static int _arg2f(const char *str, float *f2f);
static int _cpufreq_set(char *c_freq);

struct spank_option spank_options[] =
{
    {
        "cpufreq",
        "[frequency]",
        "Lock CPU at a given frequency (GHz)",
        1,
        0,
        _cpufreq_opt_process
    },
    SPANK_OPTIONS_TABLE_END
};

int slurm_spank_init(spank_t sp, int ac, char **av)
{
    if (spank_context() == S_CTX_ALLOCATOR) {
        spank_option_register(sp, spank_options);
    }
    return ESPANK_SUCCESS;
}

int slurm_spank_init_post_opt(spank_t sp, int ac, char **av)
{
    if (spank_context () != S_CTX_REMOTE) {
        return 0;
    } 

    if (freq == FREQ_NOT_SET) {
        char val[12];
        int rc;
        rc = spank_getenv(sp, FREQ_ENV_VAR, val, sizeof(val));
        if (rc)
            return rc; 
        rc = _arg2f(val, &freq);
    }
    char s_user_freq[7];
    // gcvt(freq, 6, s_user_freq);
    snprintf(s_user_freq, 7, "%.2f", freq);
    strcat (s_user_freq, "G");
    _cpufreq_set(s_user_freq);
    return ESPANK_SUCCESS;
}

static int _arg2f(const char *str, float *f2f)
{
    float l;
    char *p = NULL;
    if (!str || str[0] == '\0'){
        slurm_error("Invalid empty argument");
        return ESPANK_BAD_ARG;
    }
    l = strtof(str, &p);
    if (!p || (*p != '\0')){
        return ESPANK_BAD_ARG;
    }
    if (l < 0) {
        slurm_error("CPU Frequency must be positive"); 
        return ESPANK_BAD_ARG;
    }
    *f2f = l;
    return ESPANK_SUCCESS;
}

static int _cpufreq_opt_process(int val, const char *optarg, int remote)
{
    int rc;
    if ((rc = _arg2f(optarg, &freq)))
        return rc;
    return ESPANK_SUCCESS;
}

static int _cpufreq_set(char *c_freq){
    char *cmdName = "cpupower";
    char *argsFQ[] = {cmdName, "frequency-set", "-f", c_freq, NULL};
    pid_t c_pid = fork();
    if (c_pid ==0){
        execvp(cmdName, argsFQ);
    }
    else{
        int returnStatus;
        waitpid(c_pid, &returnStatus, 0);
        if (returnStatus == 0){
           slurm_debug("CPU FREQUENCY FIXED for PID %d\n", c_pid);
        }
    }
    return 0;
}
