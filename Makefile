# Copyright 2015 Óscar Pereira
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

PROJECT	=msmtp-notify
VERSION	=1.02

CFLAGS	?=-Wall
LDFLAGS	?=-s

all: ${PROJECT}

${PROJECT}: ${PROJECT}.c
	${CC} ${CFLAGS} -o ${PROJECT} ${PROJECT}.c ${LDFLAGS} `pkg-config --cflags --libs libnotify`

clean:
	rm -f ${PROJECT} ${PROJECT}.1* ${PROJECT}-${VERSION}.tar.gz
