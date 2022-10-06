#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <slurm/spank.h>
#include <sys/wait.h>

SPANK_PLUGIN(gpufreqspank, 1)

#define FREQ_ENV_VAR "SLURM_GPU_FREQ"
#define FREQ_NOT_SET -1
#define min_freq 200
static int freq = FREQ_NOT_SET;
static int _gpufreq_opt_process(int val, const char *optarg, int remote);
static int _arg2f(const char *str, int *f2int);
static int _nvidia_smi_set(char *c_freq);
static int _nvidia_smi_reset(void);

struct spank_option spank_options[] =
{
    {
        "gpufreq",
        "[frequency]",
        "Lock GPU at a given frequency (MHz)",
        1,
        0,
        _gpufreq_opt_process
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
    int rc;
    if (freq == FREQ_NOT_SET) {
        char val[512];
        rc = spank_getenv(sp, FREQ_ENV_VAR, val, sizeof(val));
        if (rc)
            return rc;
        rc = _arg2f(val, &freq);
    }
    if (freq < min_freq)
        freq = min_freq;
    char s_user_freq[6];
    sprintf(s_user_freq, "%d", freq);
    _nvidia_smi_set(s_user_freq);
    return 0;
}

int slurm_spank_job_epilog(spank_t sp, int ac, char **av)
{   
    _nvidia_smi_reset();
    return 0;
}

static int _arg2f(const char *str, int *f2int)
{
    long l;
    char *p = NULL;
    if (!str || str[0] == '\0'){
        slurm_error("Invalid empty argument");
        return ESPANK_BAD_ARG;
    }
    l = strtol(str, &p, 10);
    if (!p || (*p != '\0'))
        return ESPANK_BAD_ARG;
    if (l < 0) {
        slurm_error("GPU Frequency must be positive");
        return ESPANK_BAD_ARG;
    }
    *f2int = (int) l;
    return ESPANK_SUCCESS;
}

static int _gpufreq_opt_process(int val, const char *optarg, int remote)
{
    int rc;
    if ((rc = _arg2f(optarg, &freq)))
        return rc;
    return ESPANK_SUCCESS;
}

static int _nvidia_smi_set(char *c_freq){
    char *cmdName = "nvidia-smi";
    char *argsFQ[] = {cmdName, "-lgc", c_freq, NULL};
    pid_t c_pid = fork();
    if (c_pid ==0){
        execvp(cmdName, argsFQ);
    }
    else{
        int returnStatus;
        waitpid(c_pid, &returnStatus, 0);
        if (returnStatus == 0){
           slurm_debug("GPU FREQUENCY FIXED for PID %d\n", c_pid);
        }
    }
    return 0;
}

static int _nvidia_smi_reset(void){
    char *cmdName = "nvidia-smi";
    char *argsFQ[] = {cmdName, "-rgc", NULL};
    pid_t c_pid = fork();
    if (c_pid == 0){
        execvp(cmdName, argsFQ);
    }
    else{
        int returnStatus;
        waitpid(c_pid, &returnStatus, 0);
    }
    return 0;
}
