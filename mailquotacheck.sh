#!/bin/sh


exit 0
# Qmail - mailquotacheck
#
# Author: Paul Gregg <pgregg@pgregg.com>
# Url: http://www.pgregg.com/projects/qmail/
# Run this program ala: |mailquotacheck   before your ./Maildir/ or ./Mailbox
# entry in the .qmail file. e.g:
# |/usr/local/bin/mailquotacheck
# ./Maildir/

# Default quota is set to 3000Kb per user, this can be changed below

# You can also install per user quotas which will override the defaults
# by creating a .quota file in the same directory as the .qmail file, e.g:
# echo 10240 > .quota
# this will give that user a 10Mb mail quota

# Program location defs:
cat="/bin/cat"
date="/bin/date"
expr="/usr/bin/expr"
grep="/bin/grep"
wc="/usr/bin/wc"
du="/usr/bin/du"
bc="/usr/bin/bc"
cut="/usr/bin/cut"
awk="/usr/bin/awk"
echo="/bin/echo"
pwd="/bin/pwd"

# Valor por defecto de quota para el usuario, si no posee un archivo .quota
quota=51200

# Trata de determinar el tama�o del mail
msgbytes=`$cat - | $wc -c`
ERROR=$?
if [ ${ERROR} -ne 0 ]; then
	# If this fails then you are in trouble ;) - Check program defs at the top.
	$echo "QUOTACHECK ERROR: El programa que chequea las quotas de mail asignadas al usuario, no puede \
determinar el tama�o de este mensaje. Por favor, informe de esta anomal�a al postmaster del sitio al \
que est� intentado contactar."
exit 100
fi

msgkb=`$expr $msgbytes / 1024`
# or you can use:
# msgkb=`$echo $msgbytes / 1024 | $bc`

# Obtiene el directorio personal del usuario donde reside el archivo .qmail
dir="$HOME"

# Trata de determinar el valor de quota para el usuario, sino utiliza el valor por defecto
# definido m�s arriba
# If there is a file '.quota' in their dir then use that value instead.
if [ -f "$dir/.quota" ]; then
	quota=`$cat $dir/.quota 2>/dev/null`
	ERROR=$?
	if [ ${ERROR} -ne 0 ]; then
		$echo "Ha ocurrido un error cuando se intentaba conocer el tama�o de la quota del usuario. \
La entrega del mensaje se intentar� m�s tarde."
		exit 111
	fi
	if [ ${quota} -eq 0 ]; then
		# La quota vale 0. No est� limitado.
		exit 0
	fi
fi

# Determina cuanto est� ocupando el maildir del usuario
du=`$du -sk $dir | $awk {'print $1'}`
ERROR=$?
if [ ${ERROR} -ne 0 ]; then
	$echo "Ha ocurrido un error cuando se intentaba conocer el tama�o de la quota del usuario.\
La entrega del mensaje se intentar� m�s tarde."
	exit 111
fi
duwould=`$expr $du + $msgkb`

# Si el usuario est� ocupando un tama�o mayor a su quota, el mensaje no ser� entregado,
# asi que responde al remitente avisando de esta situaci�n. Adem�s avisa al destinatario
# que los mails que le llegan est�n siendo rebotados
if [ $du -gt $quota ]; then
	fecha=`$date +%s`
	usuario=`$pwd | $cut -d "/" -f 5`
	fecha_mail=`$date +"%d %b %Y %H:%M:%S %z"`

	echo -e "Date: $fecha_mail
From: postmaster@acs-multivoice.com
To: $usuario@acs-multivoice.com
Subject: Casilla de correo excedida

Han intentado enviar un mail a su direcci�n, pero este
ha sido rebotado porque su casilla excede el tama�o permitido
asignado por usuario. Por favor, elimine correo antiguo de su
casilla y vac�e las carpetas donde se almacena el mail eliminado.

" > $dir/Maildir/new/$fecha.mx1

	$echo "Usuario excedido en tama�o en disco de su cuenta de mail. \
Lamentablemente, tu mensaje no podr� ser entregado a esta cuenta. \
El espacio en disco para este email esta siendo limitado."

	exit 100
fi

exit 0
