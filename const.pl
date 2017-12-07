while(<>) {
 if(/^\#define\s+(KEY_[^\s]+)/) {
    print "$1\t\tLITERAL1\n";
    }
}