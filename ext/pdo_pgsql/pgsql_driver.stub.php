<?php

/** @generate-function-entries */

/**
 * These are extension methods for PDO. This is not a real class.
 * @undocumentable
 */
class PDO_PGSql_Ext {
    /** @tentative-return-type */
    public function pgsqlCopyFromArray(string $tableName, array | Traversable $rows, string $separator = "\t", string $nullAs = "\\\\N", ?string $fields = null): bool {}

    /** @tentative-return-type */
    public function pgsqlCopyFromFile(string $tableName, string $filename, string $separator = "\t", string $nullAs = "\\\\N", ?string $fields = null): bool {}

    /** @tentative-return-type */
    public function pgsqlCopyToArray(string $tableName, string $separator = "\t", string $nullAs = "\\\\N", ?string $fields = null): array|false {}

    /** @tentative-return-type */
    public function pgsqlCopyToFile(string $tableName, string $filename, string $separator = "\t", string $nullAs = "\\\\N", ?string $fields = null): bool {}

    /** @tentative-return-type */
    public function pgsqlLOBCreate(): string|false {}

    /** @return resource|false */
    public function pgsqlLOBOpen(string $oid, string $mode = "rb") {}

    /** @tentative-return-type */
    public function pgsqlLOBUnlink(string $oid): bool {}

    /** @tentative-return-type */
    public function pgsqlGetNotify(int $fetchMode = PDO::FETCH_DEFAULT, int $timeoutMilliseconds = 0): array|false {}

    /** @tentative-return-type */
    public function pgsqlGetPid(): int {}

    /* Do NOT add new methods here. See https://wiki.php.net/rfc/pdo_driver_specific_subclasses
     * Any new feature should be declared only on Pdo\Pgsql.
     */
}
