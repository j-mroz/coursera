/*
 * Ranks airlines by on-time arrival performance.
 *
 * Author: Jaroslaw Mroz
 */

airline_ontime_csv = LOAD 'airline_ontime.csv' USING PigStorage(',')
    AS (FlightDate:     chararray,
        UniqueCarrier:  chararray,
        FlightNum:      long,
        Origin:         chararray,
        Dest:           chararray,
        CRSDepTime:     int,
        DepDelay:       int,
        CRSArrTime:     int,
        ArrDelay:       float);

lookup_paths = LOAD 'q2_paths.csv' USING PigStorage(',')
    AS (src: chararray, dst: chararray);

flights = FOREACH airline_ontime_csv {
    GENERATE Origin AS src, Dest AS dst, ArrDelay AS delay;
}

flights_by_path = GROUP flights BY (src, dst);
flights_by_lookup_paths = JOIN flights_by_path BY (group.src, group.dst),
                               lookup_paths BY (src, dst);
avg_delay_on_lookup_paths = FOREACH flights_by_lookup_paths {
    GENERATE group.src, group.dst, AVG(flights.delay);
}

DUMP avg_delay_on_lookup_paths;
