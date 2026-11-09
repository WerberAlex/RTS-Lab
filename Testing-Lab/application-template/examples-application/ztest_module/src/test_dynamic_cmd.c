/*
 * SPDX-License-Identifier: Apache-2.0
 *
 * Tests Ztest des commandes « dynamic » de shell_module/src/dynamic_cmd.c.
 *
 * Le fichier testé est celui de l'exemple shell_module (aucune copie). Les commandes
 * sont exécutées dans le firmware, via le backend shell « dummy » : pas d'UART, pas de
 * Robot. Le même firmware tourne ensuite dans Renode (carte stm32f7), lancé par Twister.
 *
 * Complémentarité avec les tests Robot (shell_module/shell_module_advanced.robot) :
 *  - Robot pilote le shell de l'extérieur, par l'UART, comme un utilisateur ;
 *  - Ztest appelle le shell de l'intérieur et vérifie aussi les codes de retour, les
 *    limites internes (20 commandes, 32 caractères) et des cas longs à jouer par l'UART.
 * Les messages attendus sont volontairement les mêmes que dans le test Robot.
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zephyr/shell/shell_dummy.h>
#include <zephyr/ztest.h>

/* Valeurs définies (en privé) dans dynamic_cmd.c. */
#define MAX_CMD_CNT 20
#define MAX_CMD_LEN 33

/* Noms utilisés par les tests : tous sont retirés après chaque test (voir cleanup()). */
#define NB_CMDS_TESTS 25 /* cmd00 à cmd24 */

/* Sortie de la dernière commande exécutée avec run(). */
static char last_output[CONFIG_SHELL_BACKEND_DUMMY_BUF_SIZE];

/*
 * Exécute une commande sur le backend dummy et conserve sa sortie dans last_output.
 * shell_backend_dummy_get_output() vide le tampon du backend à chaque lecture : on ne
 * la lit donc qu'une fois, ici, pour pouvoir relire la sortie autant de fois que voulu.
 */
static int run(const char *cmd)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();
	size_t size;
	const char *out;
	int ret;

	shell_backend_dummy_clear_output(sh);
	ret = shell_execute_cmd(sh, cmd);

	out = shell_backend_dummy_get_output(sh, &size);
	snprintf(last_output, sizeof(last_output), "%s", out);

	return ret;
}

static const char *output(void)
{
	return last_output;
}

/* Exécute la commande puis vérifie le code de retour et un extrait de la sortie. */
static void expect(const char *cmd, int expected_ret, const char *expected_text)
{
	int ret = run(cmd);

	zassert_equal(ret, expected_ret, "'%s' a renvoye %d, attendu %d (sortie : '%s')", cmd, ret,
		      expected_ret, output());

	if (expected_text != NULL) {
		zassert_not_null(strstr(output(), expected_text),
				 "'%s' : '%s' absent de la sortie '%s'", cmd, expected_text,
				 output());
	}
}

/* Construit un nom de commande de `len` caractères (lettres 'a'). */
static void make_name(char *buf, size_t len)
{
	memset(buf, 'a', len);
	buf[len] = '\0';
}

/*
 * Retire toutes les commandes que les tests ont pu ajouter, pour que chaque test reparte
 * d'une table vide (l'état de dynamic_cmd.c est global au firmware).
 *
 * Le retrait se fait du dernier nom au premier (la table est triée) : cmd_dynamic_remove()
 * déplace une ligne de trop avec memmove(), donc retirer une commande autre que la dernière
 * d'une table pleine (20 commandes) lirait 33 octets au-delà du tableau. En partant de la
 * fin, on ne retire jamais ailleurs qu'en dernière position quand la table est pleine.
 */
static void cleanup(void *fixture)
{
	char cmd[64];
	char name[MAX_CMD_LEN];

	ARG_UNUSED(fixture);

	snprintf(cmd, sizeof(cmd), "dynamic remove test1");
	(void)run(cmd);

	for (int i = NB_CMDS_TESTS - 1; i >= 0; i--) {
		snprintf(cmd, sizeof(cmd), "dynamic remove cmd%02d", i);
		(void)run(cmd);
	}

	make_name(name, MAX_CMD_LEN - 1);
	snprintf(cmd, sizeof(cmd), "dynamic remove %s", name);
	(void)run(cmd);

	(void)run("dynamic remove beta");
	(void)run("dynamic remove alpha");
}

/* Le backend dummy démarre après le noyau : on attend qu'il soit prêt. */
static void *shell_setup(void)
{
	const struct shell *sh = shell_backend_dummy_get_ptr();

	WAIT_FOR(shell_ready(sh), 20000, k_msleep(1));
	zassert_true(shell_ready(sh), "timed out waiting for dummy shell backend");

	return NULL;
}

ZTEST_SUITE(dynamic_cmd, NULL, shell_setup, NULL, cleanup, NULL);

ZTEST(dynamic_cmd, test_show_without_commands_warns)
{
	expect("dynamic show", -ENOEXEC, "Please add some commands first.");
}

ZTEST(dynamic_cmd, test_add_then_show)
{
	expect("dynamic add alpha", 0, "command added successfully");
	expect("dynamic show", 0, "Dynamic command list:");
	zassert_not_null(strstr(output(), "[  0] alpha"), "alpha absent de la liste : '%s'",
			 output());
}

ZTEST(dynamic_cmd, test_commands_are_sorted_alphabetically)
{
	expect("dynamic add test1", 0, "command added successfully");
	expect("dynamic add alpha", 0, "command added successfully");
	expect("dynamic add beta", 0, "command added successfully");

	expect("dynamic show", 0, "Dynamic command list:");
	zassert_not_null(strstr(output(), "[  0] alpha"), "rang 0 incorrect : '%s'", output());
	zassert_not_null(strstr(output(), "[  1] beta"), "rang 1 incorrect : '%s'", output());
	zassert_not_null(strstr(output(), "[  2] test1"), "rang 2 incorrect : '%s'", output());
}

ZTEST(dynamic_cmd, test_execute_known_and_unknown_command)
{
	expect("dynamic add alpha", 0, "command added successfully");

	expect("dynamic execute alpha", 0, "dynamic command: alpha");
	expect("dynamic execute nope", -ENOEXEC, "execute: unknown parameter: nope");
}

ZTEST(dynamic_cmd, test_rejects_invalid_names)
{
	expect("dynamic add bad-name", -ENOEXEC, "please use only alphanumerical characters");
	expect("dynamic add under_score", -ENOEXEC, "bad command name");

	/* Aucune des deux commandes refusées ne doit apparaître dans la table. */
	expect("dynamic show", -ENOEXEC, "Please add some commands first.");
}

ZTEST(dynamic_cmd, test_rejects_duplicates)
{
	expect("dynamic add alpha", 0, "command added successfully");
	expect("dynamic add alpha", -ENOEXEC, "duplicated command");
}

ZTEST(dynamic_cmd, test_checks_parameter_count)
{
	/* Erreur détectée par le shell lui-même, avant d'appeler le gestionnaire. */
	expect("dynamic add", -EINVAL, "wrong parameter count");
	expect("dynamic add one two", -EINVAL, "wrong parameter count");
}

ZTEST(dynamic_cmd, test_name_length_limit)
{
	char name[MAX_CMD_LEN + 1];
	char cmd[MAX_CMD_LEN + 32];

	/* 32 caractères : accepté. */
	make_name(name, MAX_CMD_LEN - 1);
	snprintf(cmd, sizeof(cmd), "dynamic add %s", name);
	expect(cmd, 0, "command added successfully");

	/* 33 caractères : refusé. */
	make_name(name, MAX_CMD_LEN);
	snprintf(cmd, sizeof(cmd), "dynamic add %s", name);
	expect(cmd, -ENOEXEC, "too long command");
}

ZTEST(dynamic_cmd, test_table_is_limited_to_20_commands)
{
	char cmd[32];

	for (int i = 0; i < MAX_CMD_CNT; i++) {
		snprintf(cmd, sizeof(cmd), "dynamic add cmd%02d", i);
		expect(cmd, 0, "command added successfully");
	}

	/* Table pleine. */
	expect("dynamic add cmd20", -ENOEXEC, "command limit reached");

	expect("dynamic show", 0, "Dynamic command list:");
	zassert_not_null(strstr(output(), "[ 19] cmd19"), "dernier rang absent : '%s'", output());

	/* Libérer une place permet d'ajouter à nouveau. */
	expect("dynamic remove cmd19", 0, "command removed successfully");
	expect("dynamic add cmd20", 0, "command added successfully");
}

ZTEST(dynamic_cmd, test_remove_keeps_remaining_commands_in_order)
{
	expect("dynamic add alpha", 0, "command added successfully");
	expect("dynamic add beta", 0, "command added successfully");
	expect("dynamic add test1", 0, "command added successfully");

	expect("dynamic remove beta", 0, "command removed successfully");
	expect("dynamic remove nope", -ENOEXEC, "did not find command: nope");

	expect("dynamic show", 0, "Dynamic command list:");
	zassert_not_null(strstr(output(), "[  0] alpha"), "alpha absent : '%s'", output());
	zassert_not_null(strstr(output(), "[  1] test1"), "test1 absent : '%s'", output());
	zassert_is_null(strstr(output(), "beta"), "beta toujours present : '%s'", output());
}
