// SPDX-License-Identifier: MIT
// Copyright (c) 2025 AppLovin. All rights reserved.

// Handle applovin properties files

package core

import (
	"bufio"
	"errors"
	"fmt"
	"io"
	"os"
	"sort"
	"strconv"
	"strings"
	"time"

	log "github.com/sirupsen/logrus"
)

type Properties = map[string]string

func loadProperties(reader io.Reader) (Properties, error) {
	properties := make(Properties)

	scanner := bufio.NewScanner(reader)
	for scanner.Scan() {
		fullLine := scanner.Text()

		// Trim leading spaces
		line := strings.Trim(fullLine, " ")

		// Skip comments
		if strings.HasPrefix(line, "#") || len(line) == 0 {
			continue
		}

		tokens := strings.Split(line, "=")

		if len(tokens) != 2 {
			log.Printf("Invalid properties line: %s\n", line)
			continue
		}
		properties[tokens[0]] = tokens[1]
	}

	return properties, scanner.Err()
}

func LoadPropertiesFile(path string) (Properties, error) {
	properties := make(Properties)

	file, err := os.Open(path)
	if err != nil {
		return properties, err
	}
	defer file.Close()

	return loadProperties(file)
}

// Some properties are essential, we cannot run without them
func validateProperties(properties Properties) error {
	requiredProperties := []string{
		"datamover.destination_folder",
		"datamover.base_urls",
	}

	for _, name := range requiredProperties {
		entry, ok := properties[name]
		if !ok {
			return errors.New("Missing required property " + name)
		}

		if len(entry) == 0 {
			return errors.New("Required property is empty: " + name)
		}
	}

	return nil
}

func SerializeProperties(properties Properties) string {
	//
	// Filter out deprecated or properties added at runtime for debugging
	//
	keys := make([]string, 0, len(properties))

	for k := range properties {
		keys = append(keys, k)
	}
	sort.Strings(keys)

	content := ""
	for _, key := range keys {
		content += fmt.Sprintf("%s=%s\n", key, properties[key])
	}

	return content
}

func WritePropertiesFile(properties Properties, path string) error {
	return WriteStringToFile(path, SerializeProperties(properties))
}

func UpdateProperties(properties Properties, arg string) error {
	tokens := strings.Split(arg, "=")
	if len(tokens) != 2 {
		return errors.New("Invalid properties value: " + arg)
	}
	properties[tokens[0]] = tokens[1]
	return nil
}

func HasProperty(properties Properties, key string) bool {
	if _, ok := properties[key]; ok {
		return true
	} else {
		return false
	}
}

func GetIntProperty(properties Properties, key string, defaultValue int) int {
	if entry, ok := properties[key]; ok {
		val, err := strconv.Atoi(entry)
		if err == nil {
			return val
		} else {
			return defaultValue
		}
	} else {
		return defaultValue
	}
}

func GetFloat64Property(properties Properties, key string, defaultValue float64) float64 {
	if entry, ok := properties[key]; ok {
		val, err := strconv.ParseFloat(entry, 32)
		if err == nil {
			return val
		} else {
			return defaultValue
		}
	} else {
		return defaultValue
	}
}

func GetStringProperty(properties Properties, key string, defaultValue string) string {
	if entry, ok := properties[key]; ok {
		return entry
	} else {
		return defaultValue
	}
}

func GetBoolProperty(properties Properties, key string, defaultValue bool) bool {
	if entry, ok := properties[key]; ok {
		if entry == "true" {
			return true
		} else if entry == "false" {
			return false
		} else {
			log.Printf("Invalid boolean properties value: %s, should be true or false\n", entry)
			return defaultValue
		}
	} else {
		return defaultValue
	}
}

func GetDurationProperty(properties Properties, key string, defaultValue time.Duration) time.Duration {
	if entry, ok := properties[key]; ok {
		val, err := time.ParseDuration(entry)
		if err == nil {
			return val
		} else {
			return defaultValue
		}
	} else {
		return defaultValue
	}
}
