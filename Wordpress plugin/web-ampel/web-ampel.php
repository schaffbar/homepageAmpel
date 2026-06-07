<?php
/**
 * Plugin Name:       Web-Ampel
 * Plugin URI:        https://github.com/voyagersoft/esp-web-ampel
 * Description:       Ein Plugin, das einen Shortcode für eine Ampel-Anzeige basierend auf einem API-Status bereitstellt und eine Einstellungsseite im Admin-Bereich hat.
 * Version:           1.1.0
 * Author:            Steffen Wagner
 * Author URI:        https://github.com/voyagersoft/esp-web-ampel
 * License:           GPL-2.0+
 * License URI:       http://www.gnu.org/licenses/gpl-2.0.txt
 * Text Domain:       web-ampel
 */

if ( ! defined( 'ABSPATH' ) ) {
    exit;
}

// =========================================================================
// SHORTCODE-FUNKTIONALITÄT
// =========================================================================

function get_ampel_status() {
    $status = get_option('ampel_status', 'red');
    return $status;
}

function ampel_shortcode($atts) {
    // Parse shortcode attributes
    $attributes = shortcode_atts(
        array(
            'showtime' => '0', // Standard: Zeit nicht anzeigen
        ),
        $atts
    );
    
    $status = get_ampel_status();
    
    // Prüfe zunächst, ob ein aktueller Text (API-Nachricht) gesetzt ist
    $current_text = get_option('ampel_plugin_text_current', '');
    
    if ( ! empty( $current_text ) ) {
        $text = $current_text;
    } else {
        // Wähle den passenden Text basierend auf dem aktuellen Status
        switch ($status) {
            case 'green':
                $text = get_option('ampel_plugin_text_green', 'Grün: Alle Systeme funktionieren.');
                break;
            case 'yellow':
                $text = get_option('ampel_plugin_text_yellow', 'Gelb: Warnung, Überprüfung erforderlich.');
                break;
            case 'red':
                $text = get_option('ampel_plugin_text_red', 'Rot: Systemfehler, keine Funktion.');
                break;
            case 'redyellow':
                $text = get_option('ampel_plugin_text_redyellow', 'Rot-Gelb: Bereit zum Wechsel nach Grün.');
                break;
            default:
                $text = 'Status unbekannt.';
                break;
        }
    }
    
    // Hole den Zeitstempel der letzten API-Nachricht
    $message_timestamp = get_option('ampel_message_timestamp', 0);

    ob_start();
    ?>
    <div class="ampel-container">
        <!-- Rotes Licht ist jetzt oben -->
        <span class="ampel-light red-light <?php echo (($status == 'red' || $status == 'redyellow') ? 'active' : ''); ?>"></span>
        <!-- Gelbes Licht bleibt in der Mitte -->
        <span class="ampel-light yellow-light <?php echo (($status == 'yellow' || $status == 'redyellow') ? 'active' : ''); ?>"></span>
        <!-- Grünes Licht ist jetzt unten -->
        <span class="ampel-light green-light <?php echo ($status == 'green' ? 'active' : ''); ?>"></span>
    </div>
    <!-- Container für den Ampel-Status-Text -->
    <div class="ampel-text">
        <p><?php echo esc_html($text); ?></p>
        <?php if ( $attributes['showtime'] == '1' && $message_timestamp > 0 ): ?>
            <p class="ampel-message-timestamp"><small>API-Nachricht gesendet: <?php echo date_i18n( get_option( 'date_format' ) . ' ' . get_option( 'time_format' ), $message_timestamp ); ?></small></p>
        <?php endif; ?>
    </div>
    <?php
    $html = ob_get_clean();
    return $html;
}

add_shortcode('web-ampel', 'ampel_shortcode');

function ampel_plugin_styles() {
    wp_enqueue_style(
        'ampel-styles',
        plugin_dir_url(__FILE__) . 'css/ampel.css'
    );
}
add_action('wp_enqueue_scripts', 'ampel_plugin_styles');

// =========================================================================
// BENUTZERDEFINIERTE REST-API ENDPUNKT-FUNKTIONALITÄT
// =========================================================================

function register_ampel_status_endpoint() {
    register_rest_route( 'ampel/v1', '/status', array(
        'methods' => 'POST',
        'callback' => 'set_ampel_status',
        'permission_callback' => 'check_ampel_status_permission'
    ) );
}
add_action( 'rest_api_init', 'register_ampel_status_endpoint' );

function set_ampel_status( WP_REST_Request $request ) {
    $status = sanitize_text_field( $request->get_param( 'status' ) );
    $message = sanitize_text_field( $request->get_param( 'message' ) );
    
    // 'redyellow' zum Array der erlaubten Statuswerte hinzugefügt
    if ( ! in_array( $status, array( 'green', 'yellow', 'red', 'redyellow' ) ) ) {
        return new WP_REST_Response( array( 'message' => 'Ungültiger Statuswert.' ), 400 );
    }
    
    update_option( 'ampel_status', $status );
    
    // Speichere die Nachricht in ampel_plugin_text_current, falls vorhanden
    if ( ! empty( $message ) ) {
        update_option( 'ampel_plugin_text_current', $message );
    } else {
        // Wenn keine Nachricht gesendet wird, lösche den aktuellen Text
        update_option( 'ampel_plugin_text_current', '' );
    }
    update_option( 'ampel_message_timestamp', current_time( 'timestamp' ) );
    
    $response_data = array( 
        'message' => 'Status erfolgreich aktualisiert.', 
        'status' => $status 
    );
    
    if ( ! empty( $message ) ) {
        $response_data['received_message'] = $message;
    } else {
        $response_data['received_message'] = '';
    }
    
    return new WP_REST_Response( $response_data, 200 );
}

function check_ampel_status_permission( WP_REST_Request $request ) {
    $secret_key = get_option('ampel_plugin_api_secret');
    if ( empty( $secret_key ) ) {
        return new WP_Error( 'rest_forbidden', 'Der API-Schlüssel wurde nicht konfiguriert.', array( 'status' => 401 ) );
    }
    $auth_header = $request->get_header( 'X-API-KEY' );
    if ( empty( $auth_header ) || $auth_header !== $secret_key ) {
        return new WP_Error( 'rest_forbidden', 'Ungültiger API-Schlüssel.', array( 'status' => 401 ) );
    }
    return true;
}

// =========================================================================
// ADMIN-MENÜ UND EINSTELLUNGEN
// =========================================================================

function ampel_plugin_add_admin_menu() {
    add_options_page(
        'Web-Ampel Einstellungen',
        'Web-Ampel',
        'manage_options',
        'web-ampel',
        'ampel_plugin_options_page_html'
    );
}
add_action('admin_menu', 'ampel_plugin_add_admin_menu');

function ampel_plugin_settings_init() {
    // Registriere die Einstellung für den API-Schlüssel
    register_setting('web-ampel', 'ampel_plugin_api_secret');

    // Registriere die Einstellungen für die Textfelder
    register_setting('web-ampel', 'ampel_plugin_text_green');
    register_setting('web-ampel', 'ampel_plugin_text_yellow');
    register_setting('web-ampel', 'ampel_plugin_text_red');
    register_setting('web-ampel', 'ampel_plugin_text_redyellow');
    register_setting('web-ampel', 'ampel_plugin_text_current');


    add_settings_section(
        'ampel_plugin_main_section', // ID
        'API-Einstellungen',         // Titel
        null,                        // Callback-Funktion (kann null sein)
        'web-ampel'               // Seite
    );

    add_settings_field(
        'ampel_plugin_api_secret_field', // ID
        'API Secret Key',                // Titel
        'ampel_plugin_api_secret_callback', // Callback-Funktion zum Rendern des Feldes
        'web-ampel',                  // Seite
        'ampel_plugin_main_section'      // Sektion
    );

    add_settings_section(
        'ampel_plugin_text_section', // ID für die neue Sektion
        'Ampel-Status-Texte',        // Titel der Sektion
        'ampel_plugin_text_section_callback', // Callback-Funktion
        'web-ampel'               // Seite
    );

    // Füge die neuen Textfelder hinzu
    add_settings_field(
        'ampel_plugin_text_green_field',
        'Text für Grün',
        'ampel_plugin_text_green_callback',
        'web-ampel',
        'ampel_plugin_text_section'
    );
    add_settings_field(
        'ampel_plugin_text_yellow_field',
        'Text für Gelb',
        'ampel_plugin_text_yellow_callback',
        'web-ampel',
        'ampel_plugin_text_section'
    );
    add_settings_field(
        'ampel_plugin_text_red_field',
        'Text für Rot',
        'ampel_plugin_text_red_callback',
        'web-ampel',
        'ampel_plugin_text_section'
    );
    add_settings_field(
        'ampel_plugin_text_redyellow_field',
        'Text für Rot-Gelb',
        'ampel_plugin_text_redyellow_callback',
        'web-ampel',
        'ampel_plugin_text_section'
    );
    add_settings_field(
        'ampel_plugin_text_current_field',
        'Aktueller Text (API-Nachricht)',
        'ampel_plugin_text_current_callback',
        'web-ampel',
        'ampel_plugin_text_section'
    );
}
add_action('admin_init', 'ampel_plugin_settings_init');

// Callback-Funktion zum Rendern des Eingabefeldes für den API-Schlüssel
function ampel_plugin_api_secret_callback() {
    $secret = get_option('ampel_plugin_api_secret');
    ?>
    <input type="text" name="ampel_plugin_api_secret" value="<?php echo esc_attr($secret); ?>" size="50">
    <p class="description">Gib den geheimen Schlüssel ein, der für die API-Anfragen von externen Programmen verwendet wird.</p>
    <?php
}

// Callback für die neue Sektion
function ampel_plugin_text_section_callback() {
    echo '<p>Hier kannst du die Texte festlegen, die für jeden Ampel-Status angezeigt werden sollen.</p>';
}

// Callback-Funktionen für die neuen Textfelder
function ampel_plugin_text_green_callback() {
    $text = get_option('ampel_plugin_text_green', 'Grün: Alle Systeme funktionieren.');
    ?>
    <input type="text" name="ampel_plugin_text_green" value="<?php echo esc_attr($text); ?>" size="50">
    <?php
}

function ampel_plugin_text_yellow_callback() {
    $text = get_option('ampel_plugin_text_yellow', 'Gelb: Warnung, Überprüfung erforderlich.');
    ?>
    <input type="text" name="ampel_plugin_text_yellow" value="<?php echo esc_attr($text); ?>" size="50">
    <?php
}

function ampel_plugin_text_red_callback() {
    $text = get_option('ampel_plugin_text_red', 'Rot: Systemfehler, keine Funktion.');
    ?>
    <input type="text" name="ampel_plugin_text_red" value="<?php echo esc_attr($text); ?>" size="50">
    <?php
}

function ampel_plugin_text_redyellow_callback() {
    $text = get_option('ampel_plugin_text_redyellow', 'Rot-Gelb: Bereit zum Wechsel nach Grün.');
    ?>
    <input type="text" name="ampel_plugin_text_redyellow" value="<?php echo esc_attr($text); ?>" size="50">
    <?php
}

function ampel_plugin_text_current_callback() {
    $text = get_option('ampel_plugin_text_current', '');
    ?>
    <input type="text" name="ampel_plugin_text_current" value="<?php echo esc_attr($text); ?>" size="50">
    <p class="description">Dieser Text wird über die API gesetzt und überschreibt temporär die Standard-Texte.</p>
    <?php
}

// HTML-Struktur der Einstellungsseite
function ampel_plugin_options_page_html() {
    if ( ! current_user_can( 'manage_options' ) ) {
        return;
    }
    ?>
    <div class="wrap">
        <h1><?php echo esc_html( get_admin_page_title() ); ?></h1>
        
        <!-- Shortcode Usage Information -->
        <div class="card" style="max-width: none; margin-bottom: 20px;">
            <h2>Shortcode Verwendung</h2>
            <p>Verwende den folgenden Shortcode, um die Web-Ampel auf deiner Website anzuzeigen:</p>
            
            <h3>Basis-Shortcode:</h3>
            <code>[web-ampel]</code>
            <p class="description">Zeigt die Ampel mit dem aktuellen Status und Text an.</p>
            
            <h3>Shortcode mit Optionen:</h3>
            <table class="form-table">
                <tbody>
                    <tr>
                        <td><strong>Option</strong></td>
                        <td><strong>Beispiel</strong></td>
                        <td><strong>Beschreibung</strong></td>
                    </tr>
                    <tr>
                        <td><code>showtime</code></td>
                        <td><code>[web-ampel showtime="1"]</code></td>
                        <td>Zeigt den Zeitstempel der letzten API-Nachricht an (Standard: 0 = versteckt)</td>
                    </tr>
                </tbody>
            </table>
            
            <h3>Vollständige Beispiele:</h3>
            <ul>
                <li><code>[web-ampel]</code> - Standard-Anzeige ohne Zeitstempel</li>
                <li><code>[web-ampel showtime="1"]</code> - Mit Zeitstempel der letzten API-Nachricht</li>
                <li><code>[web-ampel showtime="0"]</code> - Explizit ohne Zeitstempel</li>
            </ul>
        </div>
        
        <form action="options.php" method="post">
            <?php
            settings_fields('web-ampel');
            do_settings_sections('web-ampel');
            submit_button('Einstellungen speichern');
            ?>
        </form>
    </div>
    <?php
}

