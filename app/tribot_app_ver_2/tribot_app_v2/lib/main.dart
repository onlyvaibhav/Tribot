// ============================================
// TRIBOT PROFESSIONAL CONTROLLER (Fixed)
// Compatible with: Latest Flutter SDK & "Complete TriBot Code"
// ============================================

import 'dart:async';
import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import 'package:http/http.dart' as http;
import 'package:shared_preferences/shared_preferences.dart';

void main() {
  WidgetsFlutterBinding.ensureInitialized();
  SystemChrome.setPreferredOrientations([
    DeviceOrientation.portraitUp,
    DeviceOrientation.portraitDown,
  ]);
  runApp(const TriBotApp());
}

class TriBotApp extends StatelessWidget {
  const TriBotApp({super.key});

  @override
  Widget build(BuildContext context) {
    return MaterialApp(
      title: 'Tribot', // App title
      debugShowCheckedModeBanner: false,
      theme: ThemeData(
        useMaterial3: true,
        colorSchemeSeed: Colors.cyan,
        brightness: Brightness.dark,
        scaffoldBackgroundColor: const Color(0xFF0A0E21),
      ),
      // CHANGE: Point home to the Splash Screen first
      home: const SplashScreen(),
    );
  }
}

// ============================================
// SPLASH SCREEN ANIMATION
// ============================================
class SplashScreen extends StatefulWidget {
  const SplashScreen({super.key});

  @override
  State<SplashScreen> createState() => _SplashScreenState();
}

class _SplashScreenState extends State<SplashScreen> with SingleTickerProviderStateMixin {
  late AnimationController _controller;
  late Animation<double> _scaleAnimation;
  late Animation<double> _opacityAnimation;

  @override
  void initState() {
    super.initState();

    // 1. Setup Animation Controller
    _controller = AnimationController(
      duration: const Duration(seconds: 2),
      vsync: this,
    );

    // 2. Define Scale (Bouncing Effect)
    _scaleAnimation = Tween<double>(begin: 0.5, end: 1.0).animate(
      CurvedAnimation(parent: _controller, curve: Curves.elasticOut),
    );

    // 3. Define Fade (Text appearing)
    _opacityAnimation = Tween<double>(begin: 0.0, end: 1.0).animate(
      CurvedAnimation(parent: _controller, curve: const Interval(0.5, 1.0, curve: Curves.easeIn)),
    );

    // 4. Start Animation
    _controller.forward();

    // 5. Navigate to App after 3 seconds
    Timer(const Duration(seconds: 3), () {
      Navigator.of(context).pushReplacement(
        MaterialPageRoute(builder: (_) => const TriBotHome()),
      );
    });
  }

  @override
  void dispose() {
    _controller.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      backgroundColor: const Color(0xFF0A0E21),
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          children: [
            // Animated Icon
            ScaleTransition(
              scale: _scaleAnimation,
              child: Container(
                padding: const EdgeInsets.all(30),
                decoration: BoxDecoration(
                  shape: BoxShape.circle,
                  boxShadow: [
                    BoxShadow(
                      color: Colors.cyan.withOpacity(0.3),
                      blurRadius: 40,
                      spreadRadius: 10,
                    )
                  ],
                  border: Border.all(color: Colors.cyanAccent, width: 2),
                ),
                child: const Icon(
                  Icons.smart_toy_outlined, // Robot Icon
                  size: 80,
                  color: Colors.cyanAccent,
                ),
              ),
            ),
            const SizedBox(height: 30),
            // Animated Text
            FadeTransition(
              opacity: _opacityAnimation,
              child: const Column(
                children: [
                  Text(
                    "TRIBOT",
                    style: TextStyle(
                      fontSize: 32,
                      fontWeight: FontWeight.bold,
                      letterSpacing: 5,
                      color: Colors.white,
                    ),
                  ),
                  SizedBox(height: 10),
                  Text(
                    "SYSTEM INITIALIZING...",
                    style: TextStyle(
                      fontSize: 12,
                      color: Colors.cyan,
                      letterSpacing: 2,
                    ),
                  ),
                ],
              ),
            ),
          ],
        ),
      ),
    );
  }
}

// ============================================
// ROBOT SERVICE (Networking)
// ============================================
class RobotService {
  static final RobotService _instance = RobotService._internal();
  factory RobotService() => _instance;
  RobotService._internal();

  final http.Client _client = http.Client();
  String _host = 'http://192.168.4.1'; // Default ESP8266 IP
  
  bool _isSending = false;
  DateTime _lastCommandTime = DateTime.now();
  
  static const _minCommandInterval = Duration(milliseconds: 50);
  
  String get host => _host;
  set host(String value) => _host = value.trim();

  Future<bool> sendCommand(String param, String value) async {
    final now = DateTime.now();
    bool isSpeedCmd = value.length == 1 && int.tryParse(value) != null;
    
    if (!isSpeedCmd && now.difference(_lastCommandTime) < _minCommandInterval) {
      return true; 
    }
    _lastCommandTime = now;

    if (_isSending) return false;
    _isSending = true;

    try {
      final uri = Uri.parse('$_host/action?$param=$value');
      final response = await _client.get(uri).timeout(
        const Duration(milliseconds: 1500),
      );
      return response.statusCode == 200;
    } catch (e) {
      return false;
    } finally {
      _isSending = false;
    }
  }

  Future<bool> sendState(String value) => sendCommand('State', value);
  Future<bool> sendMode(String value) => sendCommand('Mode', value);
  
  Future<bool> testConnection() async {
    try {
      final uri = Uri.parse('$_host/');
      final response = await _client.get(uri).timeout(const Duration(seconds: 2));
      return response.statusCode == 200;
    } catch (e) {
      return false;
    }
  }
}

// ============================================
// MAIN HOME SCREEN
// ============================================
class TriBotHome extends StatefulWidget {
  const TriBotHome({super.key});

  @override
  State<TriBotHome> createState() => _TriBotHomeState();
}

class _TriBotHomeState extends State<TriBotHome> with SingleTickerProviderStateMixin {
  final RobotService _robot = RobotService();
  late TabController _tabController;
  
  bool _isConnected = false;
  String _status = 'Not connected';
  int _speed = 5; 
  int _currentTabIndex = 0;
  String? _runningAutoMode;

  @override
  void initState() {
    super.initState();
    _tabController = TabController(length: 3, vsync: this);
    _tabController.addListener(_onTabChanged);
    _loadSettings();
  }

  @override
  void dispose() {
    _tabController.removeListener(_onTabChanged);
    _tabController.dispose();
    super.dispose();
  }

  void _onTabChanged() {
    if (_tabController.indexIsChanging) return;
    
    final newIndex = _tabController.index;
    if (newIndex == _currentTabIndex) return;
    
    _currentTabIndex = newIndex;
    
    if (_runningAutoMode != null) {
      _stopAllModes();
    }
    
    if (newIndex == 0) {
      _enableManualMode();
    }
  }

  Future<void> _loadSettings() async {
    try {
      final prefs = await SharedPreferences.getInstance();
      final savedHost = prefs.getString('host');
      final savedSpeed = prefs.getInt('speed');
      if (savedHost != null) _robot.host = savedHost;
      if (savedSpeed != null) {
        setState(() => _speed = savedSpeed);
        _robot.sendState(_speed.toString());
      }
      _checkConnection();
    } catch (_) {}
  }

  Future<void> _saveSpeed(int newSpeed) async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.setInt('speed', newSpeed);
  }

  Future<void> _checkConnection() async {
    final connected = await _robot.testConnection();
    if (mounted) {
      setState(() {
        _isConnected = connected;
        _status = connected ? 'Connected' : 'Connection failed';
      });
    }
  }

  Future<void> _enableManualMode() async {
    final success = await _robot.sendMode('W');
    if (mounted) {
      setState(() {
        _isConnected = success;
        _status = success ? 'Manual Mode Ready' : 'Cmd Failed';
        _runningAutoMode = null;
      });
    }
  }

  Future<void> _startAutoMode(String mode) async {
    HapticFeedback.mediumImpact();
    final success = await _robot.sendMode(mode);
    
    if (success) {
      await _robot.sendState(_speed.toString());
    }

    if (mounted) {
      setState(() {
        _isConnected = success;
        if (success) {
          _runningAutoMode = mode;
          _status = 'Running: ${_getModeName(mode)}';
        } else {
          _runningAutoMode = null;
          _status = 'Failed to start';
        }
      });
    }
  }

  Future<void> _stopAllModes() async {
    await _robot.sendMode('X'); 
    await _robot.sendState('S'); 
    if (mounted) {
      setState(() {
        _runningAutoMode = null;
        _status = 'Stopped';
      });
    }
  }

  Future<void> _sendMoveCommand(String command) async {
    HapticFeedback.selectionClick();
    await _robot.sendState(command);
  }

  Future<void> _updateSpeed(int newSpeed) async {
    setState(() => _speed = newSpeed);
    await _robot.sendState(newSpeed.toString());
    _saveSpeed(newSpeed);
  }

  Future<void> _emergencyStop() async {
    HapticFeedback.heavyImpact();
    await _stopAllModes();
    if (mounted) {
      setState(() => _status = 'EMERGENCY STOP');
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('EMERGENCY STOP SENT'), backgroundColor: Colors.red),
      );
    }
  }

  String _getModeName(String code) {
    if (code == 'O') return 'Obstacle Avoidance';
    if (code == 'L') return 'Line Follower';
    return 'Unknown';
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('TriBot Pro'),
        centerTitle: true,
        actions: [
          IconButton(icon: const Icon(Icons.wifi), onPressed: _checkConnection),
          IconButton(
            icon: const Icon(Icons.settings),
            onPressed: () => Navigator.push(
              context, 
              MaterialPageRoute(builder: (_) => SettingsPage(
                host: _robot.host,
                onHostChanged: (h) { _robot.host = h; _checkConnection(); }
              ))
            ),
          ),
        ],
        bottom: TabBar(
          controller: _tabController,
          tabs: const [
            Tab(icon: Icon(Icons.gamepad), text: 'Manual'),
            Tab(icon: Icon(Icons.radar), text: 'Obstacle'),
            Tab(icon: Icon(Icons.timeline), text: 'Line'),
          ],
        ),
      ),
      body: Column(
        children: [
          _StatusBar(status: _status, isConnected: _isConnected, isRunning: _runningAutoMode != null),
          Expanded(
            child: TabBarView(
              controller: _tabController,
              physics: const NeverScrollableScrollPhysics(),
              children: [
                ManualModePage(
                  speed: _speed,
                  onCommand: _sendMoveCommand,
                  onSpeedChanged: _updateSpeed,
                  onActivate: _enableManualMode,
                ),
                AutoModePage(
                  title: 'Obstacle Avoidance',
                  desc: 'Robot drives forward at selected speed. When blocked, it backs up and turns.',
                  icon: Icons.radar,
                  modeCode: 'O',
                  speed: _speed,
                  isRunning: _runningAutoMode == 'O',
                  onStart: () => _startAutoMode('O'),
                  onStop: _stopAllModes,
                  onSpeedChanged: _updateSpeed,
                ),
                AutoModePage(
                  title: 'Line Follower',
                  desc: 'Follows black line. Use speed slider to adjust for friction/smoothness.',
                  icon: Icons.timeline,
                  modeCode: 'L',
                  speed: _speed,
                  isRunning: _runningAutoMode == 'L' || _runningAutoMode == 'F',
                  onStart: () => _startAutoMode('L'),
                  onStop: _stopAllModes,
                  onSpeedChanged: _updateSpeed,
                ),
              ],
            ),
          ),
        ],
      ),
      floatingActionButton: FloatingActionButton.extended(
        onPressed: _emergencyStop,
        backgroundColor: Colors.redAccent,
        icon: const Icon(Icons.stop_circle_outlined, size: 32),
        label: const Text("STOP", style: TextStyle(fontWeight: FontWeight.bold)),
      ),
    );
  }
}

// ============================================
// MANUAL PAGE
// ============================================
class ManualModePage extends StatelessWidget {
  final int speed;
  final Function(String) onCommand;
  final Function(int) onSpeedChanged;
  final VoidCallback onActivate;

  const ManualModePage({
    super.key,
    required this.speed,
    required this.onCommand,
    required this.onSpeedChanged,
    required this.onActivate,
  });

  @override
  Widget build(BuildContext context) {
    return SingleChildScrollView(
      padding: const EdgeInsets.all(16),
      child: Column(
        children: [
          const Text("WIFI CONTROL", style: TextStyle(color: Colors.cyan, letterSpacing: 2, fontWeight: FontWeight.bold)),
          const SizedBox(height: 20),
          
          Container(
            padding: const EdgeInsets.all(16),
            decoration: BoxDecoration(
              color: Colors.black26,
              borderRadius: BorderRadius.circular(30),
              // FIXED: .withOpacity -> .withValues
              border: Border.all(color: Colors.cyan.withValues(alpha: 0.3)),
            ),
            child: Column(
              children: [
                Row(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    _Btn(icon: Icons.north_west, cmd: 'G', onCmd: onCommand),
                    const SizedBox(width: 10),
                    _Btn(icon: Icons.arrow_upward, cmd: 'A', onCmd: onCommand, isBig: true),
                    const SizedBox(width: 10),
                    _Btn(icon: Icons.north_east, cmd: 'I', onCmd: onCommand),
                  ],
                ),
                const SizedBox(height: 10),
                Row(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    _Btn(icon: Icons.arrow_back, cmd: 'L', onCmd: onCommand, isBig: true),
                    const SizedBox(width: 10),
                    _StopBtn(onCmd: onCommand),
                    const SizedBox(width: 10),
                    _Btn(icon: Icons.arrow_forward, cmd: 'R', onCmd: onCommand, isBig: true),
                  ],
                ),
                const SizedBox(height: 10),
                Row(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    _Btn(icon: Icons.south_west, cmd: 'H', onCmd: onCommand),
                    const SizedBox(width: 10),
                    _Btn(icon: Icons.arrow_downward, cmd: 'B', onCmd: onCommand, isBig: true),
                    const SizedBox(width: 10),
                    _Btn(icon: Icons.south_east, cmd: 'J', onCmd: onCommand),
                  ],
                ),
              ],
            ),
          ),
          const SizedBox(height: 20),
          SpeedSlider(speed: speed, onChanged: onSpeedChanged),
          const SizedBox(height: 20),
          ElevatedButton.icon(
            onPressed: onActivate, 
            icon: const Icon(Icons.refresh), 
            label: const Text("Reset / Enable Manual")
          )
        ],
      ),
    );
  }
}

// ============================================
// AUTO MODE PAGE
// ============================================
class AutoModePage extends StatelessWidget {
  final String title;
  final String desc;
  final IconData icon;
  final String modeCode;
  final int speed;
  final bool isRunning;
  final VoidCallback onStart;
  final VoidCallback onStop;
  final Function(int) onSpeedChanged;

  const AutoModePage({
    super.key,
    required this.title,
    required this.desc,
    required this.icon,
    required this.modeCode,
    required this.speed,
    required this.isRunning,
    required this.onStart,
    required this.onStop,
    required this.onSpeedChanged,
  });

  @override
  Widget build(BuildContext context) {
    return SingleChildScrollView(
      padding: const EdgeInsets.all(16),
      child: Column(
        children: [
          Icon(icon, size: 60, color: isRunning ? Colors.greenAccent : Colors.grey),
          const SizedBox(height: 10),
          Text(title, style: const TextStyle(fontSize: 24, fontWeight: FontWeight.bold)),
          const SizedBox(height: 10),
          Text(desc, textAlign: TextAlign.center, style: const TextStyle(color: Colors.white70)),
          const SizedBox(height: 30),
          
          Row(
            children: [
              Expanded(
                child: ElevatedButton.icon(
                  onPressed: isRunning ? null : onStart,
                  style: ElevatedButton.styleFrom(
                    // FIXED: .withOpacity -> .withValues
                    backgroundColor: Colors.green.withValues(alpha: 0.8),
                    foregroundColor: Colors.white,
                    padding: const EdgeInsets.symmetric(vertical: 16),
                  ),
                  icon: const Icon(Icons.play_arrow),
                  label: const Text("START"),
                ),
              ),
              const SizedBox(width: 16),
              Expanded(
                child: ElevatedButton.icon(
                  onPressed: isRunning ? onStop : null,
                  style: ElevatedButton.styleFrom(
                    // FIXED: .withOpacity -> .withValues
                    backgroundColor: Colors.red.withValues(alpha: 0.8),
                    foregroundColor: Colors.white,
                    padding: const EdgeInsets.symmetric(vertical: 16),
                  ),
                  icon: const Icon(Icons.stop),
                  label: const Text("STOP"),
                ),
              ),
            ],
          ),
          const SizedBox(height: 30),
          
          Container(
            padding: const EdgeInsets.all(16),
            decoration: BoxDecoration(
              color: Colors.black26,
              borderRadius: BorderRadius.circular(16),
              border: Border.all(color: Colors.white10)
            ),
            child: Column(
              children: [
                const Text("LIVE SPEED ADJUSTMENT", style: TextStyle(fontWeight: FontWeight.bold, color: Colors.cyanAccent)),
                const SizedBox(height: 8),
                const Text("Adjust slider below to change speed in real-time.", style: TextStyle(fontSize: 12, color: Colors.white54)),
                SpeedSlider(speed: speed, onChanged: onSpeedChanged),
              ],
            ),
          ),
        ],
      ),
    );
  }
}

// ============================================
// REUSABLE WIDGETS
// ============================================

class SpeedSlider extends StatelessWidget {
  final int speed;
  final Function(int) onChanged;

  const SpeedSlider({super.key, required this.speed, required this.onChanged});

  @override
  Widget build(BuildContext context) {
    return Column(
      children: [
        Padding(
          padding: const EdgeInsets.symmetric(horizontal: 10),
          child: Row(
            mainAxisAlignment: MainAxisAlignment.spaceBetween,
            children: [
              const Text("Slow"),
              Text("Level $speed", style: const TextStyle(fontSize: 18, fontWeight: FontWeight.bold, color: Colors.cyan)),
              const Text("Fast"),
            ],
          ),
        ),
        Slider(
          value: speed.toDouble(),
          min: 0,
          max: 9,
          divisions: 9,
          activeColor: Colors.cyan,
          onChanged: (val) => onChanged(val.round()),
        ),
      ],
    );
  }
}

class _Btn extends StatelessWidget {
  final IconData icon;
  final String cmd;
  final Function(String) onCmd;
  final bool isBig;

  const _Btn({required this.icon, required this.cmd, required this.onCmd, this.isBig = false});

  @override
  Widget build(BuildContext context) {
    double size = isBig ? 70 : 55;
    return GestureDetector(
      onTapDown: (_) => onCmd(cmd),
      onTapUp: (_) => onCmd('S'),
      onTapCancel: () => onCmd('S'),
      child: Container(
        width: size, height: size,
        decoration: BoxDecoration(
          // FIXED: .withOpacity -> .withValues
          color: isBig ? Colors.cyan.withValues(alpha: 0.2) : Colors.white10,
          borderRadius: BorderRadius.circular(15),
          border: Border.all(color: isBig ? Colors.cyan : Colors.white24),
        ),
        child: Icon(icon, size: isBig ? 32 : 24, color: Colors.white),
      ),
    );
  }
}

class _StopBtn extends StatelessWidget {
  final Function(String) onCmd;
  const _StopBtn({required this.onCmd});

  @override
  Widget build(BuildContext context) {
    return GestureDetector(
      onTap: () { HapticFeedback.mediumImpact(); onCmd('S'); },
      child: Container(
        width: 70, height: 70,
        decoration: BoxDecoration(
          // FIXED: .withOpacity -> .withValues
          color: Colors.red.withValues(alpha: 0.3),
          borderRadius: BorderRadius.circular(15),
          border: Border.all(color: Colors.red),
        ),
        child: const Icon(Icons.stop, size: 32, color: Colors.redAccent),
      ),
    );
  }
}

class _StatusBar extends StatelessWidget {
  final String status;
  final bool isConnected;
  final bool isRunning;
  const _StatusBar({required this.status, required this.isConnected, required this.isRunning});

  @override
  Widget build(BuildContext context) {
    Color color = isRunning ? Colors.green : (isConnected ? Colors.cyan : Colors.orange);
    return Container(
      width: double.infinity,
      padding: const EdgeInsets.all(8),
      // FIXED: .withOpacity -> .withValues
      color: color.withValues(alpha: 0.1),
      child: Row(
        mainAxisAlignment: MainAxisAlignment.center,
        children: [
          Icon(Icons.wifi, size: 16, color: color),
          const SizedBox(width: 8),
          Text(status, style: TextStyle(color: color, fontWeight: FontWeight.bold)),
        ],
      ),
    );
  }
}

// ============================================
// SETTINGS PAGE
// ============================================
class SettingsPage extends StatefulWidget {
  final String host;
  final ValueChanged<String> onHostChanged;
  const SettingsPage({super.key, required this.host, required this.onHostChanged});

  @override
  State<SettingsPage> createState() => _SettingsPageState();
}

class _SettingsPageState extends State<SettingsPage> {
  late TextEditingController _ctrl;
  @override
  void initState() {
    super.initState();
    _ctrl = TextEditingController(text: widget.host);
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: const Text("Settings")),
      body: Padding(
        padding: const EdgeInsets.all(20),
        child: Column(
          children: [
            TextField(
              controller: _ctrl,
              decoration: const InputDecoration(
                labelText: "Robot IP",
                hintText: "http://192.168.4.1",
                border: OutlineInputBorder(),
              ),
            ),
            const SizedBox(height: 20),
            ElevatedButton(
              onPressed: () {
                widget.onHostChanged(_ctrl.text);
                Navigator.pop(context);
              },
              child: const Text("SAVE"),
            )
          ],
        ),
      ),
    );
  }
}