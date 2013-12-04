#!/system/bin/sh

exec_disable_sbinric(){
	$1/busybox pkill /sbin/ric
	/system/bin/mount -o remount,rw /
	/system/bin/rm /sbin/ric
	/system/bin/mount -o remount,ro /
	$1/busybox pkill /sbin/ric

	/system/bin/mount -o remount,rw /system

	/system/bin/mkdir /system/etc/init.d
	/system/bin/chown root.root /system/etc/init.d
	/system/bin/chmod 0755 /system/etc/init.d

	if grep "/system/xbin/busybox run-parts /system/etc/init.d" /system/etc/hw_config.sh > /dev/null; then
		:
	else
		echo "/system/xbin/busybox run-parts /system/etc/init.d" >> /system/etc/hw_config.sh
	fi

	/system/bin/dd if=$1/00stop_ric of=/system/etc/init.d/00stop_ric
	/system/bin/chown root.root /system/etc/init.d/00stop_ric
	/system/bin/chmod 0755 /system/etc/init.d/00stop_ric

}

exec_disable_systemric(){
	stop ric
	/system/bin/mount -o remount,rw -t ext4 /dev/block/mmcblk0p12 /system
	/system/bin/rm /system/bin/ric
}

file_copy(){

}

export DEVICE=`getprop ro.product.device`
export ID=`getprop ro.build.id`

case $ID in
6.1.F.0.106)
exec_disable_systemric
;;
6.1.D.1.74)
exec_disable_systemric
;;
6.1.D.1.91)
exec_disable_systemric
;;
6.1.F.0.117)
exec_disable_sbinric $1
;;
6.1.F.0.128)
exec_disable_sbinric $1
;;
6.1.D.1.103)
exec_disable_sbinric $1
;;
9.1.C.1.103)
exec_disable_sbinric $1
;;
10.3.1.D.0.220)
exec_disable_sbinric $1
;;
esac

/system/bin/mount -o remount,rw /system

/system/bin/dd if=$1/su of=/system/xbin/su
/system/bin/chown root.root /system/xbin/su
/system/bin/chmod 06755 /system/xbin/su
/system/bin/ln -s /system/xbin/su /system/bin/su

/system/bin/dd if=$1/Superuser.apk of=/system/app/Superuser.apk
/system/bin/chown root.root /system/app/Superuser.apk
/system/bin/chmod 0644 /system/app/Superuser.apk

/system/bin/dd if=$1/busybox of=/system/xbin/busybox
/system/bin/chown root.shell /system/xbin/busybox
/system/bin/chmod 04755 /system/xbin/busybox
/system/xbin/busybox --install -s /system/xbin

/system/bin/mount -o remount,ro /system
