
#!/bin/sh
# set DISTRO either to "Ubuntu", "Debian" or "Fedora"
DISTRO="Fedora"
LINVERSION="18.04"

# Get Jamulus Release Name with "curl" and "grep"  
R=`curl -s https://api.github.com/repos/jamulussoftware/jamulus/releases/latest | grep -oP '"tag_name": "\K(.*)(?=")'` 
echo "Jamulus Installation Script for $DISTRO $LINVERSION"
echo "Release: $R"
INSTALLJAMULUS="no"
while true; do
    read -p "Do you wish to install Jamulus on $DISTRO $LINVERSION? (y/n) " yn
    case $yn in
        [Yy]* ) 
           echo "Start Installation $DISTRO $LINVERSION"
           echo "(1) Fetch Release $R from GitHub"
           wget https://github.com/jamulussoftware/jamulus/archive/$R.tar.gz 
           echo "(2) Extract Source Code for Jamulus Release $R from GitHub"
           tar -xvf $R.tar.gz
           echo "(3) Delete ${R}.tar.gz from GitHub"
           rm $R.tar.gz
           echo "(4) Update Repository"
           sudo apt-get update
           INSTALLJAMULUS="yes" 
           break;;
        [Nn]* ) 
           echo "Cancelled Jamulus Installation on $DISTRO $LINVERSION"
           exit;;
        * ) echo "Please answer yes or no.";;
    esac
done

# echo "Check Variable: $INSTALLJAMULUS"
	
if [ "$INSTALLJAMULUS" = "yes" ]; then     
	echo "(5) Install Build Essentials for $DISTRO"
	
	if [ "$DISTRO"  = "Ubuntu" ]
	then  
		      echo "Installation for $DISTRO" 
		      sudo apt-get install cmake qmake gcc g++ 
		      sudo apt-get install build-essential qt5-qmake qtdeclarative5-dev qt5-default qttools5-dev-tools libjack-jackd2-dev 
		      sudo apt-get install qjackctl
                      if [ "$LINVERSION"  = "18.4" ]
                      then
                          echo "Perform Installation Specifics for $DISTRO Version $DISTRO" 
                      fi  
	
  	elif [ "$DISTRO" = "Debian" ]
	then    
			  sudo apt-get install build-essential qtdeclarative5-dev qt5-default qttools5-dev-tools libjack-jackd2-dev 
			  sudo apt-get install qjackctl
	elif [ "$DISTRO" = "Fedora" ]
	then    
			  sudo dnf install qt5-qtdeclarative-devel jack-audio-connection-kit-dbus jack-audio-connection-kit-devel 
			  sudo dnf install qjackctl
	fi
           
	echo "(6) Compile Jamulus $R"
	echo "Change to Directory jamulus-$R" 
	cd "jamulus-$R"
	# ls
	qmake Jamulus.pro
	make clean
        make
        sudo make install
        echo "Compilation DONE"
        cd ..
        echo "(6) Delete the Source Files after Installation"
        rm -R "jamulus-$R"
           
else

	echo "Installation cancelled"

fi

