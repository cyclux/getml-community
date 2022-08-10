// Copyright 2022 The SQLNet Company GmbH
// 
// This file is licensed under the Elastic License 2.0 (ELv2). 
// Refer to the LICENSE.txt file in the root of the repository 
// for details.
// 

package install

import (
	"io/ioutil"
	"os"
	"path/filepath"
)

func copyDir(sourceDir string, mainDir string, overwrite bool) {

	destinationDir := filepath.Join(mainDir, sourceDir)

	os.MkdirAll(destinationDir, os.ModePerm)

	files, err := ioutil.ReadDir(sourceDir)

	if err != nil {
		panic(err)
	}

	for _, file := range files {
		fname := filepath.Join(sourceDir, file.Name())
		copyFile(fname, "", mainDir, overwrite)
	}

}
