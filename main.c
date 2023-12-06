#include "app_ui/app_ui.h"
#include <getopt.h>

static struct option long_options[] = {{"help",    no_argument, NULL, 'h'},
                                       {"version", no_argument, NULL, 'v'},
                                       {0}};

static void printHelp(const char *programName);

int main(int argc, char **argv) {
	int c, longIndex = 0;
	while ((c = getopt_long(argc, argv, ":hv", long_options, &longIndex)) != -1) {
		switch (c) {
			case 'h':
				printHelp(argv[0]);
				return 0;
			case 'v':
				printf("%s %s\n", PROJECT_NAME, PROJECT_VERSION);
				return 0;
			default:
				fprintf(stderr, "%s: Unknown option '%s'\n", argv[0], argv[optind - 1]);
				printHelp(argv[0]);
				return -1;
		}
	}

	AdwApplication *app = adw_application_new(APPLICATION_ID, G_APPLICATION_DEFAULT_FLAGS);
	g_signal_connect(app, "activate", G_CALLBACK(on_app_activate), NULL);
	return g_application_run(G_APPLICATION(app), argc, argv);
}

static void printHelp(const char *programName) {
	printf("Usage: %s [options]\n", programName);
	printf("\nOptions:\n");
	printf("  -h, --help                  Prints this page\n");
	printf("  -v, --version               Shows program version\n");
}
