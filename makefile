all: screenreader.app intensity.app

screenreader.app: FORCE
	make -C screenreader

intensity.app: FORCE
	make -C intensity

FORCE:
