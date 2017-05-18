/*
 * Counts flights to/from by the airpots code.
 *
 * Author: Jaroslaw Mroz
 */

airline_ontime = LOAD '/tmp/airline_ontime_sample.csv'
    USING PigStorage(',')
    AS (FlightDate:     chararray,
        UniqueCarrier:  chararray,
        FlightNum:      long,
        Origin:         chararray,
        Dest:           chararray,
        CRSDepTime:     int,
        DepDelay:       int,
        CRSArrTime:     int,
        ArrDelay:       int);

src_airport = FOREACH airline_ontime GENERATE Origin AS airport;
src_airport_group = GROUP src_airport BY airport;
src_airport_count = FOREACH src_airport_group
    generate group as airport, COUNT(src_airport) AS count;

dst_airport = FOREACH airline_ontime GENERATE Dest AS airport;
dst_airport_group = GROUP dst_airport BY airport;
dst_airport_count = FOREACH dst_airport_group
    generate group AS airport, COUNT(dst_airport) AS count;

airports_count_union = UNION src_airport_count, dst_airport_count;
airports_count_group = GROUP airports_count_union BY airport;
airports_count = FOREACH airports_count_group generate group AS airport,
    SUM(airports_count_union.count) AS count;

airports_rank = ORDER airports_count BY count DESC PARALLEL 4;

top10 = LIMIT airports_rank 10;
DUMP top10;
