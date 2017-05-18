/*
 * Ranks airlines by on-time arrival performance.
 *
 * Author: Jaroslaw Mroz
 */

airline_ontime_file = LOAD 'airline_ontime.csv'
    USING PigStorage(',')
    AS (FlightDate:     chararray,
        UniqueCarrier:  chararray,
        FlightNum:      long,
        Origin:         chararray,
        Dest:           chararray,
        CRSDepTime:     int,
        DepDelay:       int,
        CRSArrTime:     int,
        ArrDelay:       float);


-- airline_delay_tuple_and_header = FOREACH airline_ontime_file
--         GENERATE UniqueCarrier AS airline, ArrDelay AS delay;

airline_delay_tuple_and_header = FOREACH airline_ontime_file
        GENERATE UniqueCarrier AS airline,
                 (ArrDelay < 0.0 ? 0.0 : ArrDelay) AS delay;

airline_delay_tuple = FILTER airline_delay_tuple_and_header BY airline != 'UniqueCarrier';

airline_delay_group = GROUP airline_delay_tuple BY airline;
airline_avg_delay = FOREACH airline_delay_group
    GENERATE group AS airline, AVG(airline_delay_tuple.delay) AS avg_delay;
airline_rank_avg_delay = ORDER airline_avg_delay BY avg_delay ASC;

top10 = LIMIT airline_rank_avg_delay 10;
DUMP top10;
