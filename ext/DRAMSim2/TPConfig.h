class TPConfig {
    public :
    int num_pids;
    bool partitioning;

    int security_policy;
    int allocation_timer;
    int allocator;
    int dead_time_calc;

    class EpochSettings{
        public:
        int epoch_length;
        int *bandwidth_minimum;
        int num_pids;
        EpochSettings(int num_pids){
            bandwidth_minimum = ((int*) malloc(sizeof(int) * num_pids));
        }
    };

    EpochSettings* epoch_settings;
};
