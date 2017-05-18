// Author: Jaroslaw Mroz
// Preprocess airline ontime csv files, cuts not needed collumns,
// merges the files into one big file.

package main

import (
	"encoding/csv"
	"io"
	"log"
	"os"
	"path/filepath"
)

const inputFiles = "On_Time*.csv"
const outputFile = "airline_ontime.csv"

const recordsChannelSize = 100

// CSV airline_ontime fields.
const (
	Year = iota
	Quarter
	Month
	DayofMonth
	DayOfWeek
	FlightDate
	UniqueCarrier
	AirlineID
	Carrier
	TailNum
	FlightNum
	Origin
	OriginCityName
	OriginState
	OriginStateFips
	OriginStateName
	OriginWac
	Dest
	DestCityName
	DestState
	DestStateFips
	DestStateName
	DestWac
	CRSDepTime
	DepTime
	DepDelay
	DepDelayMinutes
	DepDel15
	DepartureDelayGroups
	DepTimeBlk
	TaxiOut
	WheelsOff
	WheelsOn
	TaxiIn
	CRSArrTime
	ArrTime
	ArrDelay
	ArrDelayMinutes
	ArrDel15
	ArrivalDelayGroups
	ArrTimeBlk
	Cancelled
	CancellationCode
	Diverted
	CRSElapsedTime
	ActualElapsedTime
	AirTime
	Flights
	Distance
	DistanceGroup
	CarrierDelay
	WeatherDelay
	NASDelay
	SecurityDelay
	LateAircraftDelay
)

// Load csv file, returns a channel of csv rows.
func loadCSVFiles(files []string) <-chan []string {
	records := make(chan []string, recordsChannelSize)

	// Read csv records in separate routine.
	go func() {
		defer close(records)

		for fidx := range files {
			file, err := os.Open(files[fidx])
			if err != nil {
				log.Fatalln("error opening file", files[fidx], err)
			}
			defer file.Close()

			reader := csv.NewReader(file)

			for {
				record, err := reader.Read()
				if err == io.EOF {
					break
				}
				if err != nil {
					log.Fatalln("error reading record to csv:", err)
				}
				records <- record
			}
		}

	}()

	return records
}

// Process provided airline_on_time csv records.
// Reads csv recodr from a input channel and writes preprocessed record
// into output channe.
func parseAirlineOntimeRecords(input <-chan []string) <-chan []string {
	output := make(chan []string, recordsChannelSize)

	// Parse csv records in separate routine.
	go func() {
		defer close(output)

		for record := range input {

			// Sip cancelled or diverted flights.
			if record[Cancelled] == "1.00" || record[Diverted] == "1.0" {
				continue
			}

			// Make sure arrival delay is present.
			if record[ArrDelay] == "" {
				record[ArrDelay] = "0.0"
			}

			output <- []string{
				record[FlightDate],
				record[UniqueCarrier],
				record[FlightNum],
				record[Origin],
				record[Dest],
				// record[AirlineID],
				record[CRSDepTime],
				// record[DepTime],
				record[DepDelay],
				record[CRSArrTime],
				// record[ArrTime],
				record[ArrDelay],
			}
		}
	}()

	return output
}

// Redds csv records from channel and seves then to output file.
func saveCSV(filePath string, records <-chan []string) {
	file, err := os.OpenFile(filePath, os.O_RDWR|os.O_CREATE, 0766)
	if err != nil {
		log.Fatalln("error opening file", filePath, err)
	}
	defer file.Close()

	writer := csv.NewWriter(file)

	for record := range records {
		err := writer.Write(record)
		if err != nil {
			log.Fatalln("error writing record to csv:", err)
		}
	}

	writer.Flush()
	if err := writer.Error(); err != nil {
		log.Fatal(err)
	}
}

// Driver function.
func main() {
	files, err := filepath.Glob(inputFiles)
	if err != nil {
		log.Fatalln("Failed to locate csv files", err)
	}

	// Processing pipline, each step done in separate coroutine.
	inRecords := loadCSVFiles(files)
	outRecords := parseAirlineOntimeRecords(inRecords)
	saveCSV(outputFile, outRecords)
}
