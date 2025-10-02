import 'package:flutter/material.dart';
import 'package:flutter/services.dart';
import '../utils/theme.dart';

class PinInputWidget extends StatefulWidget {
  final Function(String) onComplete;
  final String? errorMessage;
  final bool showForgotPin;
  final VoidCallback? onForgotPin;
  final bool canUseBiometric;
  final VoidCallback? onBiometric;

  const PinInputWidget({
    super.key,
    required this.onComplete,
    this.errorMessage,
    this.showForgotPin = false,
    this.onForgotPin,
    this.canUseBiometric = false,
    this.onBiometric,
  });

  @override
  State<PinInputWidget> createState() => _PinInputWidgetState();
}

class _PinInputWidgetState extends State<PinInputWidget>
    with TickerProviderStateMixin {
  final List<String> _pin = List.filled(6, '');
  int _currentIndex = 0;
  late AnimationController _shakeController;
  late Animation<double> _shakeAnimation;
  late AnimationController _fadeController;
  late Animation<double> _fadeAnimation;

  @override
  void initState() {
    super.initState();
    _setupAnimations();
  }

  void _setupAnimations() {
    _shakeController = AnimationController(
      duration: const Duration(milliseconds: 600),
      vsync: this,
    );

    _shakeAnimation = Tween<double>(
      begin: 0,
      end: 1,
    ).animate(CurvedAnimation(
      parent: _shakeController,
      curve: Curves.elasticIn,
    ));

    _fadeController = AnimationController(
      duration: const Duration(milliseconds: 300),
      vsync: this,
    );

    _fadeAnimation = Tween<double>(
      begin: 1,
      end: 0,
    ).animate(_fadeController);
  }

  @override
  void didUpdateWidget(PinInputWidget oldWidget) {
    super.didUpdateWidget(oldWidget);
    if (widget.errorMessage != null && oldWidget.errorMessage == null) {
      _shakeAndClear();
    }
  }

  void _shakeAndClear() {
    _shakeController.forward().then((_) {
      _shakeController.reset();
      setState(() {
        _pin.fillRange(0, _pin.length, '');
        _currentIndex = 0;
      });
    });
  }

  void _onNumberTap(String number) {
    if (_currentIndex < _pin.length) {
      setState(() {
        _pin[_currentIndex] = number;
        _currentIndex++;
      });

      // Haptic feedback
      HapticFeedback.lightImpact();

      // Check if PIN is complete
      if (_currentIndex == _pin.length) {
        final pinString = _pin.join();
        Future.delayed(const Duration(milliseconds: 200), () {
          widget.onComplete(pinString);
        });
      }
    }
  }

  void _onBackspace() {
    if (_currentIndex > 0) {
      setState(() {
        _currentIndex--;
        _pin[_currentIndex] = '';
      });
      HapticFeedback.lightImpact();
    }
  }

  void _onClear() {
    setState(() {
      _pin.fillRange(0, _pin.length, '');
      _currentIndex = 0;
    });
    HapticFeedback.lightImpact();
  }

  @override
  Widget build(BuildContext context) {
    return Column(
      children: [
        // PIN dots display
        AnimatedBuilder(
          animation: _shakeAnimation,
          builder: (context, child) {
            final offset = sin(_shakeAnimation.value * 3.14159 * 2) * 5;
            return Transform.translate(
              offset: Offset(offset, 0),
              child: Row(
                mainAxisAlignment: MainAxisAlignment.center,
                children: List.generate(6, (index) {
                  return Container(
                    margin: const EdgeInsets.symmetric(horizontal: 8),
                    width: 20,
                    height: 20,
                    decoration: BoxDecoration(
                      shape: BoxShape.circle,
                      color: _pin[index].isNotEmpty
                          ? AppTheme.primaryColor
                          : Colors.transparent,
                      border: Border.all(
                        color: widget.errorMessage != null
                            ? AppTheme.errorColor
                            : _pin[index].isNotEmpty
                                ? AppTheme.primaryColor
                                : AppTheme.textMuted,
                        width: 2,
                      ),
                    ),
                    child: _pin[index].isNotEmpty
                        ? const Icon(
                            Icons.circle,
                            size: 8,
                            color: Colors.white,
                          )
                        : null,
                  );
                }),
              ),
            );
          },
        ),
        const SizedBox(height: 16),

        // Error message
        if (widget.errorMessage != null)
          Container(
            padding: const EdgeInsets.symmetric(horizontal: 16, vertical: 8),
            decoration: BoxDecoration(
              color: AppTheme.errorColor.withOpacity(0.1),
              borderRadius: BorderRadius.circular(8),
              border: Border.all(
                color: AppTheme.errorColor.withOpacity(0.3),
              ),
            ),
            child: Text(
              widget.errorMessage!,
              style: const TextStyle(
                color: AppTheme.errorColor,
                fontSize: 14,
                fontWeight: FontWeight.w500,
              ),
              textAlign: TextAlign.center,
            ),
          ),
        const SizedBox(height: 32),

        // Number pad
        _buildNumberPad(),
        const SizedBox(height: 24),

        // Additional options
        Row(
          mainAxisAlignment: MainAxisAlignment.spaceEvenly,
          children: [
            if (widget.showForgotPin)
              TextButton(
                onPressed: widget.onForgotPin,
                child: const Text(
                  'Forgot PIN?',
                  style: TextStyle(color: AppTheme.primaryColor),
                ),
              ),
            if (widget.canUseBiometric)
              IconButton(
                onPressed: widget.onBiometric,
                icon: const Icon(
                  Icons.fingerprint,
                  color: AppTheme.primaryColor,
                  size: 32,
                ),
                tooltip: 'Use biometric authentication',
              ),
          ],
        ),
      ],
    );
  }

  Widget _buildNumberPad() {
    return Container(
      constraints: const BoxConstraints(maxWidth: 300),
      child: Column(
        children: [
          // Row 1: 1, 2, 3
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceEvenly,
            children: [
              _buildNumberButton('1'),
              _buildNumberButton('2'),
              _buildNumberButton('3'),
            ],
          ),
          const SizedBox(height: 16),
          // Row 2: 4, 5, 6
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceEvenly,
            children: [
              _buildNumberButton('4'),
              _buildNumberButton('5'),
              _buildNumberButton('6'),
            ],
          ),
          const SizedBox(height: 16),
          // Row 3: 7, 8, 9
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceEvenly,
            children: [
              _buildNumberButton('7'),
              _buildNumberButton('8'),
              _buildNumberButton('9'),
            ],
          ),
          const SizedBox(height: 16),
          // Row 4: Clear, 0, Backspace
          Row(
            mainAxisAlignment: MainAxisAlignment.spaceEvenly,
            children: [
              _buildActionButton(
                onTap: _onClear,
                child: const Icon(
                  Icons.clear,
                  color: AppTheme.textSecondary,
                ),
              ),
              _buildNumberButton('0'),
              _buildActionButton(
                onTap: _onBackspace,
                child: const Icon(
                  Icons.backspace_outlined,
                  color: AppTheme.textSecondary,
                ),
              ),
            ],
          ),
        ],
      ),
    );
  }

  Widget _buildNumberButton(String number) {
    return GestureDetector(
      onTap: () => _onNumberTap(number),
      child: Container(
        width: 70,
        height: 70,
        decoration: BoxDecoration(
          shape: BoxShape.circle,
          color: AppTheme.cardColor,
          border: Border.all(
            color: AppTheme.textMuted.withOpacity(0.3),
          ),
        ),
        child: Center(
          child: Text(
            number,
            style: const TextStyle(
              fontSize: 24,
              fontWeight: FontWeight.w600,
              color: AppTheme.textPrimary,
            ),
          ),
        ),
      ),
    );
  }

  Widget _buildActionButton({
    required VoidCallback onTap,
    required Widget child,
  }) {
    return GestureDetector(
      onTap: onTap,
      child: Container(
        width: 70,
        height: 70,
        decoration: BoxDecoration(
          shape: BoxShape.circle,
          color: Colors.transparent,
        ),
        child: Center(child: child),
      ),
    );
  }

  @override
  void dispose() {
    _shakeController.dispose();
    _fadeController.dispose();
    super.dispose();
  }
}

double sin(double value) {
  // Simple sine approximation for shake animation
  return (value % (2 * 3.14159)) / (2 * 3.14159) * 2 - 1;
}