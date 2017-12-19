enum ColumnValue {
    STATUS = 0,
    CYCLES = 1,
    SP_TEMPERATURE = 2,
    SP_INPRESSURE = 3,
    SP_OUTPRESSURE = 4,
    TEMPERATURE = 5,
    CURRENT = 6,
    INPRESSURE = 7,
    OUTPRESSURE = 8,
    FLOW = 9
};
enum StaticColumnValue{
    TIME = 0,
    OXYGEN = 3*10+1,
    OXYGEN_TEMP = 3*10+2,
    T_OXYGEN = 3*10+3,
    
};
enum ColumnStatus{
    ABSORB = 0,
    INTERME_A = 1,
    INTERME_B = 2,
    DESORB = 3,
    INVALID = 4,
};
